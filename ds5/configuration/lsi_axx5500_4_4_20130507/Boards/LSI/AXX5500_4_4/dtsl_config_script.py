from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import FormatterMode
from com.arm.debug.dtsl.components import CSDAP
from com.arm.debug.dtsl.components import MemoryRouter
from com.arm.debug.dtsl.components import DapMemoryAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.components import DeviceCluster
from com.arm.debug.dtsl.configurations.options import IIntegerOption
from com.arm.debug.dtsl.components import TMCETBTraceCapture
from com.arm.debug.dtsl.components import CSCTI
from com.arm.debug.dtsl.components import PTMTraceSource
from com.arm.debug.dtsl.components import CSFunnel
from com.arm.debug.dtsl.components import CTISyncSMPDevice
from com.arm.debug.dtsl.components import RDDISyncSMPDevice

CLUSTER_SIZES = [ 4, 4 ]
NUM_CLUSTERS = len(CLUSTER_SIZES)

ATB_ID_BASE = 2
TRACE_RANGE_DESCRIPTION = '''Limit trace capture to the specified range. This is useful for restricting trace capture to an OS (e.g. Linux kernel)'''
useCTIsForSMP = True
CTM_CHANNEL_SYNC_STOP = 0  # use channel 0 for sync stop
CTM_CHANNEL_SYNC_START = 1  # use channel 1 for sync start
CTM_CHANNEL_TRACE_TRIGGER = 2  # use channel 2 for trace triggers
CORTEX_A15_TRACE_OPTIONS = 0

class TraceRangeOptions:
    def __init__(self, cluster = None, coreTraceName = None, dtsl = None):
        if coreTraceName == None:
            self.defaultSetup()
        else:
            self.traceRangeEnable = dtsl.getOptionValue("options.trace_%d.%s.traceRange" % (cluster, coreTraceName))
            self.traceRangeStart = dtsl.getOptionValue("options.trace_%d.%s.traceRange.start" % (cluster, coreTraceName))
            self.traceRangeEnd = dtsl.getOptionValue("options.trace_%d.%s.traceRange.end" % (cluster, coreTraceName))
            self.traceRangeIDs = None
    
    def defaultSetup(self):
        self.traceRangeEnable = False
        self.traceRangeStart = None
        self.traceRangeEnd = None
        self.traceRangeIDs = None
    

class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=[
                DTSLv1.tabPage("trace_%d" % c, "Cluster %d Trace" %c, childOptions=
                    [ DTSLv1.enumOption('traceCapture', 'Trace capture method', defaultValue="none",
                          values = [("none", "None"), ("ETB_%d"%c, "On Chip Trace Buffer (ETB)")]),
                     DTSLv1.booleanOption('cortexA15coreTrace', 'Enable Cortex-A15 core trace', defaultValue=False,
                        childOptions =
                            # Allow each source to be enabled/disabled individually
                            [ DTSLv1.booleanOption('Cortex_A15_%d' % core,
                                                   "Enable Cortex-A15 %d trace" % core, defaultValue=True)
                              for core in range(CLUSTER_SIZES[c]) ] +
                            # Pull in common options for PTMs (cycle accurate etc)
                            PTMTraceSource.defaultOptions(DtslScript.getPTMs) +
                            [ # Trace range selection (e.g. for linux kernel)
                            DTSLv1.booleanOption('traceRange', 'Trace capture range',
                                description=TRACE_RANGE_DESCRIPTION,
                                defaultValue = False,
                                childOptions = [
                                    DTSLv1.integerOption('start', 'Start address',
                                        description='Start address for trace capture',
                                        defaultValue=0,
                                        display=IIntegerOption.DisplayFormat.HEX),
                                    DTSLv1.integerOption('end', 'End address',
                                        description='End address for trace capture',
                                        defaultValue=0xFFFFFFFF,
                                        display=IIntegerOption.DisplayFormat.HEX)
                                ])
                            ]
                        ),
                ])
                for c in range(NUM_CLUSTERS)
            ])
        ]
    
    def __init__(self, root):
        DTSLv1.__init__(self, root)
        
        # locate devices on the platform and create corresponding objects
        self.discoverDevices()
        
        self.mgdPlatformDevs = set()
        
        # tracks which devices are managed when a trace mode is enabled
        self.mgdTraceDevs = {}
        
        # only DAP device is managed by default - others will be added when enabling trace, SMP etc
        self.mgdPlatformDevs.add(self.dap)
        
        self.exposeCores()
        
        self.setupSMP()

        for c in range(NUM_CLUSTERS):
            self.setupETBTrace(c)
        
        # use internal trace range to limit trace to e.g. kernel
        self.traceRangeOptions = [
            TraceRangeOptions(), # Cortex-A15 trace options
            ]
        
        self.setManagedDevices(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''find and create devices'''
        
        dapDev = self.findDevice("ARMCS-DP")
        self.dap = CSDAP(self, dapDev, "DAP")
        
        cortexA15coreDev = 0
        self.cortexA15cores = [ [] for i in range(NUM_CLUSTERS) ]
        
        streamID = ATB_ID_BASE
        
        # Trace start/stop CTI
        outCTIDev = self.findDevice("CSCTI")
        self.outCTI = CSCTI(self, outCTIDev, "CTI_out")
        coreCTIDev = outCTIDev
        self.coreCTIs = [ [] for i in range(NUM_CLUSTERS) ]
        self.cortexA15ctiMap = {} # map cores to associated CTIs
        
        ptmDev = 1
        self.PTMs = [ [] for i in range(NUM_CLUSTERS) ]

        etbDev = 0
        self.ETBs = []
        
        funnelDev = 0
        self.funnels = []
        
        for c in range(NUM_CLUSTERS):
            for i in range(CLUSTER_SIZES[c]):
                # create core
                cortexA15coreDev = self.findDevice("Cortex-A15", cortexA15coreDev+1)
                dev = Device(self, cortexA15coreDev, "Cortex-A15_%d_%d" % (c, i))
                self.cortexA15cores[c].append(dev)
                
                # create CTI for this core
                coreCTIDev = self.findDevice("CSCTI", coreCTIDev+1)
                coreCTI = CSCTI(self, coreCTIDev, "CTI_%d_%d" % (c, i))
                self.coreCTIs.append(coreCTI)
                self.cortexA15ctiMap[dev] = coreCTI
                
                # create the PTM for this core
                ptmDev = self.findDevice("CSPTM", ptmDev+1)
                ptm = PTMTraceSource(self, ptmDev, streamID, "PTM_%d_%d" % (c, i))
                streamID += 1
                # disabled by default - will enable with option
                ptm.setEnabled(False)
                self.PTMs[c].append(ptm)
            
            # ETB
            etbDev = self.findDevice("CSTMC", etbDev+1)
            self.ETBs.append(TMCETBTraceCapture(self, etbDev, "ETB_%d" % c))
        
            # Funnel 0
            funnelDev = self.findDevice("CSTFunnel")
            self.funnels.append(self.createFunnel(funnelDev, "Funnel_%d" % c))

        
    def exposeCores(self):
        for cluster in self.cortexA15cores:
            for core in cluster:
                self.addDeviceInterface(self.createDAPWrapper(core))


    def createSMPDevice(self, cores, clusters, name):
        # create SMP device and expose from configuration
        if useCTIsForSMP:
            ctiInfo = {}
            ctis = []
            for c in cores:
                cti = self.cortexA15ctiMap[c]
                # use standard Cortex event mapping : in/out on trigger 0 for stop, out on trigger 7 for start
                ctiInfo[c] = CTISyncSMPDevice.DeviceCTIInfo(cti, CTISyncSMPDevice.DeviceCTIInfo.NONE, 7, 0, 0)
                ctis.append(cti)
            smp = CTISyncSMPDevice(self, name, clusters, ctiInfo, CTM_CHANNEL_SYNC_START, CTM_CHANNEL_SYNC_STOP)
            # automatically handle connection to CTIs
            self.addManagedPlatformDevices(ctis)
        else:
            smp = RDDISyncSMPDevice(self, name, clusters)
        
        self.addDeviceInterface(self.createDAPWrapper(smp))

    
    def setupSMP(self):
        '''Create SMP device using RDDI synchronization'''
        
        for c in range(NUM_CLUSTERS):
            cores = self.cortexA15cores[c]
            self.createSMPDevice(cores, cores, "Cortex-A15_%d" % c)

        clusterMap = [ DeviceCluster("Cluster %d" % c, self.cortexA15cores[c]) for c in range(NUM_CLUSTERS) ]
        allCores = [ core for cluster in self.cortexA15cores for core in cluster ]
        self.createSMPDevice(allCores, clusterMap, "Cortex-A15_all")

    
    def setupETBTrace(self, c):
        '''Setup ETB trace capture'''
        
        # use continuous mode
        self.ETBs[c].setFormatterMode(FormatterMode.CONTINUOUS)
        
        # register other trace components with ETB and register ETB with configuration
        self.ETBs[c].setTraceComponentOrder([ self.funnels[c] ])
        self.addTraceCaptureInterface(self.ETBs[c])
        
        # automatically handle connection/disconnection to trace components
        self.addManagedTraceDevices("ETB_%d" % c, [ self.funnels[c], self.outCTI, self.ETBs[c] ])
        
        # register trace sources
        self.registerTraceSources(c, self.ETBs[c])
        
    
    def getCTIForSink(self, sink):
        '''Get the CTI and input/channel associated with a trace sink
        return (None, None, None) if no associated CTI
        '''
        if sink == self.ETBs[0]:
            # ETB 0 trigger input is CTI out 1
            return (self.outCTI, 1, CTM_CHANNEL_TRACE_TRIGGER)
        elif sink == self.ETBs[1]:
            # ETB 1 trigger input is CTI out 3
            return (self.outCTI, 3, CTM_CHANNEL_TRACE_TRIGGER)

        # no associated CTI
        return (None, None, None)
    
    def getCTIForSource(self, source):
        '''Get the CTI and input/channel associated with a source
        return (None, None, None) if no associated CTI
        '''
        for c in range(NUM_CLUSTERS):
            if source in self.PTMs[c]:
                coreNum = self.PTMs[c].index(source)
                # PTM trigger is on input 6
                if coreNum < len(self.coreCTIs[c]):
                    return (self.coreCTIs[c][coreNum], 6, CTM_CHANNEL_TRACE_TRIGGER)
        
        # no associated CTI
        return (None, None, None)
    
    def setTraceSourceEnabled(self, funnel, source, enabled):
        '''Enable/disable a trace source'''
        source.setEnabled(enabled)
        self.enableFunnelPortForSource(funnel, source, enabled)
        self.enableCTIsForSource(source, enabled)
    
    def setETBTraceEnabled(self, c, enabled):
        '''Enable/disable ETB trace capture'''
        self.enableCTIsForSink(self.ETBs[c], enabled)
    
    def registerTraceSources(self, c, traceCapture):
        '''Register all trace sources with trace capture device'''
        for c in range(NUM_CLUSTERS):
            for i in range(CLUSTER_SIZES[c]):
                self.registerCoreTraceSource(traceCapture, self.cortexA15cores[c][i], self.PTMs[c][i])
        
    
    def registerCoreTraceSource(self, traceCapture, core, source):
        '''Register a trace source with trace capture device and enable triggers'''
        # Register with trace capture, associating with core
        traceCapture.addTraceSource(source, core.getID())
        
        # source is managed by the configuration
        self.addManagedTraceDevices(traceCapture.getName(), [ source ])
        
        # CTI (if present) is also managed by the configuration
        cti, input, channel = self.getCTIForSource(source)
        if cti:
            self.addManagedTraceDevices(traceCapture.getName(), [ cti ])
    
    def getFunnelPortForSource(self, source):
        '''Get the funnel port number for a trace source'''
        
        # Build map of sources to funnel ports
        portMap = {}
        for c in range(NUM_CLUSTERS):
            for i in range(CLUSTER_SIZES[c]):
                # assume cores are assigned linearly to funnel ports within each cluster
                portMap[self.PTMs[c][i]] = i
        
        return portMap.get(source, None)
    
    # +--------------------------------+
    # | Callback functions for options |
    # +--------------------------------+
    
    def optionValuesChanged(self):
        '''Callback to update the configuration state after options are changed'''
        optionValues = self.getOptionValues()
        mDevs = set()
        for c in range(NUM_CLUSTERS):
            traceMode = optionValues.get("options.trace_%d.traceCapture" % c)
            self.setTraceCaptureMethod(c, traceMode)
            mDevs = mDevs | self.getManagedDevices(traceMode)
        self.setManagedDevices(mDevs)

        ptmStartIndex = 0
        for c in range(NUM_CLUSTERS):
            coreTraceEnabled = self.getOptionValue("options.trace_%d.cortexA15coreTrace" % c)
            for i in range(CLUSTER_SIZES[c]):
                thisCoreTraceEnabled = self.getOptionValue("options.trace_%d.cortexA15coreTrace.Cortex_A15_%d" % (c, i))
                funnel = self.funnels[c]
                enableSource = coreTraceEnabled and thisCoreTraceEnabled
                self.setTraceSourceEnabled(funnel, self.PTMs[c][i], enableSource)
        
            self.setInternalTraceRange(self.traceRangeOptions[CORTEX_A15_TRACE_OPTIONS],
                                       TraceRangeOptions(c, "cortexA15coreTrace", self),
                                       self.PTMs[c])

        
    def getManagedDevices(self, traceKey):
        '''Get the required set of managed devices for this configuration'''
        return self.mgdPlatformDevs | self.mgdTraceDevs.get(traceKey, set())
    
    def setTraceCaptureMethod(self, c, method):
        if method == "none":
            self.setETBTraceEnabled(c, False)
        elif method == "ETB":
            self.setETBTraceEnabled(c, True)
    
    def getPTMs(self):
        '''Get the PTMs'''
        allPTMs = [ p for c in self.PTMs for p in c ]
        return allPTMs
    
    def setCoreTraceEnabled(self, enabled):
        '''Enable/disable the core trace sources'''
        for cluster in self.PTMs:
            for source in cluster:
                self.setTraceSourceEnabled(source, enabled)
        
    
    # +------------------------------+
    # | Target independent functions |
    # +------------------------------+
    
    def addManagedPlatformDevices(self, devs):
        '''Add devices to the set of devices managed by the configuration'''
        for d in devs:
            self.mgdPlatformDevs.add(d)
    
    def registerTraceSource(self, traceCapture, source):
        '''Register trace source with trace capture device'''
        traceCapture.addTraceSource(source)
        self.addManagedTraceDevices(traceCapture.getName(), [ source ])
    
    def addManagedTraceDevices(self, traceKey, devs):
        '''Add devices to the set of devices managed by the configuration for this trace mode'''
        traceDevs = self.mgdTraceDevs.get(traceKey)
        if not traceDevs:
            traceDevs = set()
            self.mgdTraceDevs[traceKey] = traceDevs
        for d in devs:
            traceDevs.add(d)
    
    def createDAPWrapper(self, core):
        '''Add a wrapper around a core to allow access to AHB and APB via the DAP'''
        return MemoryRouter(
            [DapMemoryAccessor("AXI", self.dap, 0, "AXI bus accessed via AP_0 on DAP_0"),
             DapMemoryAccessor("APB", self.dap, 1, "APB bus accessed via AP_1 on DAP_0")],
            core)
    
    def enableCTIInput(self, cti, input, channel, enabled):
        '''Enable/disable cross triggering between an input and a channel'''
        if enabled:
            cti.enableInputEvent(input, channel)
        else:
            cti.disableInputEvent(input, channel)
    
    def enableCTIsForSink(self, sink, enabled):
        '''Enable/disable triggers using CTI associated with source'''
        cti, output, channel = self.getCTIForSink(sink)
        if cti:
            self.enableCTIOutput(cti, output, channel, enabled)
    
    def enableCTIsForSource(self, source, enabled):
        '''Enable/disable triggers using CTI associated with source'''
        cti, input, channel = self.getCTIForSource(source)
        if cti:
            self.enableCTIInput(cti, input, channel, enabled)
    
    def enableCTIOutput(self, cti, output, channel, enabled):
        '''Enable/disable cross triggering between a channel and an output'''
        if enabled:
            cti.enableOutputEvent(output, channel)
        else:
            cti.disableOutputEvent(output, channel)
    
    def createFunnel(self, funnelDev, name):
        funnel = CSFunnel(self, funnelDev, name)
        funnel.setAllPortsDisabled() # Will enable for each source later
        return funnel
    
    def enableFunnelPortForSource(self, funnel, source, enabled):
        '''Enable/disable the funnel port for a trace source'''
        port = self.getFunnelPortForSource(source)
        if enabled:
            funnel.setPortEnabled(port)
        else:
            funnel.setPortDisabled(port)
    
    def setInternalTraceRange(self, currentTraceOptions, newTraceOptions, traceMacrocells):
        # values are different to current config
        if (newTraceOptions.traceRangeEnable != currentTraceOptions.traceRangeEnable) or \
            (newTraceOptions.traceRangeStart != currentTraceOptions.traceRangeStart) or \
            (newTraceOptions.traceRangeEnd != currentTraceOptions.traceRangeEnd):
            
            # clear existing ranges
            if currentTraceOptions.traceRangeIDs:
                for i in range(0, len(traceMacrocells)):
                    traceMacrocells[i].clearTraceRange(currentTraceOptions.traceRangeIDs[i])
                currentTraceOptions.traceRangeIDs = None
                
            # set new ranges
            if newTraceOptions.traceRangeEnable:
                currentTraceOptions.traceRangeIDs = [
                    traceMacrocells[i].addTraceRange(newTraceOptions.traceRangeStart, newTraceOptions.traceRangeEnd)
                    for i in range(0, len(traceMacrocells))
                    ]
                
            currentTraceOptions.traceRangeEnable = newTraceOptions.traceRangeEnable
            currentTraceOptions.traceRangeStart = newTraceOptions.traceRangeStart
            currentTraceOptions.traceRangeEnd = newTraceOptions.traceRangeEnd
    

