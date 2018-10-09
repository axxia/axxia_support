from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import FormatterMode
from com.arm.debug.dtsl.components import CSDAP
from com.arm.debug.dtsl.components import MemoryRouter
from com.arm.debug.dtsl.components import DapMemoryAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.configurations.options import IIntegerOption
from com.arm.debug.dtsl.components import CSTMC
from com.arm.debug.dtsl.components import TMCETBTraceCapture
from com.arm.debug.dtsl.components import ETRTraceCapture
from com.arm.debug.dtsl.components import CSCTI
from com.arm.debug.dtsl.components import ETMv4TraceSource
from com.arm.debug.dtsl.components import CSFunnel
from com.arm.debug.dtsl.components import STMTraceSource
from com.arm.debug.dtsl.components import CTISyncSMPDevice

NUM_CORES_CORTEX_A57 = 2
TRACE_RANGE_DESCRIPTION = '''Limit trace capture to the specified range. This is useful for restricting trace capture to an OS (e.g. Linux kernel)'''
CTM_CHANNEL_SYNC_STOP = 2  # Use channel 2 for sync stop
CTM_CHANNEL_SYNC_START = 1  # Use channel 1 for sync start
CTM_CHANNEL_TRACE_TRIGGER = 3  # Use channel 3 for trace triggers

# Import core specific functions
import a57_rams


class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=[
                DTSLv1.tabPage("trace", "Trace Capture", childOptions=[
                    DTSLv1.enumOption('traceCapture', 'Trace capture method', defaultValue="none",
                        values = [("none", "None"), ("ETF0", "On Chip Trace Buffer (ETF0/TMC)"), ("ETR0", "System Memory Trace Buffer (ETR0/TMC)")],
                        setter=DtslScript.setTraceCaptureMethod),
                ]),
                DTSLv1.tabPage("cortexA57", "Cortex-A57", childOptions=[
                    DTSLv1.booleanOption('coreTrace', 'Enable Cortex-A57 core trace', defaultValue=False,
                        childOptions =
                            # Allow each source to be enabled/disabled individually
                            [ DTSLv1.booleanOption('Cortex_A57_%d' % c, "Enable Cortex-A57 %d trace" % c, defaultValue=True)
                            for c in range(0, NUM_CORES_CORTEX_A57) ] +
                            [ ETMv4TraceSource.cycleAccurateOption(DtslScript.getETMsForCortex_A57) ] +
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
                ]),
                DTSLv1.tabPage("ETR", "ETR", childOptions=[
                    DTSLv1.booleanOption('etrBuffer0', 'Configure the system memory trace buffer to be used by the ETR0/TMC device', defaultValue=False,
                        childOptions = [
                            DTSLv1.integerOption('start', 'Start address',
                            description='Start address of the system memory trace buffer to be used by the ETR0/TMC device',
                            defaultValue=0x00100000,
                            display=IIntegerOption.DisplayFormat.HEX),
                            DTSLv1.integerOption('size', 'Size in Words',
                            description='Size of the system memory trace buffer in Words (total bytes/system memory width)',
                            defaultValue=0x8000,
                            isDynamic=True,
                            display=IIntegerOption.DisplayFormat.HEX),
                            DTSLv1.booleanOption('scatterGather', 'Enable scatter-gather mode',
                            defaultValue=False,
                            description='When enabling scatter-gather mode, the start address of the on-chip trace buffer must point to a configured scatter-gather table')
                        ]
                    ),
                ]),
                DTSLv1.tabPage("stm", "STM", childOptions=[
                    DTSLv1.booleanOption('STM0', 'Enable STM 0 trace', defaultValue=False),
                    DTSLv1.booleanOption('STM1', 'Enable STM 1 trace', defaultValue=False),
                ]),
                DTSLv1.tabPage("rams", "Cache RAMs", childOptions=[
                    # Turn cache debug mode on/off
                    DTSLv1.booleanOption('cacheDebug', 'Cache debug mode',
                                         description='Turning cache debug mode on enables reading the cache RAMs. Enabling it may adversely impact debug performance.',
                                         defaultValue=False, isDynamic=True),
                    DTSLv1.booleanOption('cachePreserve', 'Preserve cache contents in debug state',
                                         description='Preserve the contents of caches while the core is stopped.',
                                         defaultValue=False, isDynamic=True),
                ]),
            ])
        ]
    
    def __init__(self, root):
        DTSLv1.__init__(self, root)
        
        '''Do not add directly to this list - first check if the item you are adding is already present'''
        self.mgdPlatformDevs = []
        
        # Tracks which devices are managed when a trace mode is enabled
        self.mgdTraceDevs = {}
        
        # Locate devices on the platform and create corresponding objects
        self.discoverDevices()
        
        # Only DAP device is managed by default - others will be added when enabling trace, SMP etc
        if self.dap not in self.mgdPlatformDevs:
            self.mgdPlatformDevs.append(self.dap)
        
        self.exposeCores()
        
        self.traceRangeIDs = {}
        
        traceComponentOrder = [ self.Funnel0 ]
        managedDevices = [ self.Funnel0, self.OutCTI0, self.ETF0Trace ]
        self.setupETFTrace(self.ETF0Trace, "ETF0", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel0, self.ETF0 ]
        managedDevices = [ self.Funnel0, self.ETF0, self.OutCTI0, self.ETR0 ]
        self.setupETRTrace(self.ETR0, "ETR0", traceComponentOrder, managedDevices)
        
        self.setupCTISyncSMP()
        
        self.setManagedDeviceList(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''Find and create devices'''
        
        self.dap = CSDAP(self, 1, "DAP")
        
        cortexA57coreDevs = [4, 8]
        self.cortexA57cores = []
        
        streamID = 1
        
        # Trace start/stop CTI 0
        self.OutCTI0 = CSCTI(self, 3, "OutCTI0")
        
        coreCTIDevs = [5, 9]
        self.CoreCTIs = []
        
        etmDevs = [7, 18]
        self.ETMs = []
        
        # STM 0
        self.STM0 = self.createSTM(10, streamID, "STM0")
        streamID += 1
        
        # STM 1
        self.STM1 = self.createSTM(13, streamID, "STM1")
        streamID += 1
        
        for i in range(0, NUM_CORES_CORTEX_A57):
            # Create core
            core = a57_rams.A57CoreDevice(self, cortexA57coreDevs[i], "Cortex-A57_%d" % i)
            self.cortexA57cores.append(core)
            
        for i in range(0, len(coreCTIDevs)):
            # Create CTI
            coreCTI = CSCTI(self, coreCTIDevs[i], "CoreCTIs[%d]" % i)
            self.CoreCTIs.append(coreCTI)
            
        for i in range(0, len(etmDevs)):
            # Create ETM
            etm = self.createETM(etmDevs[i], streamID, "ETMs[%d]" % i)
            streamID += 1
            
        tmcDev = 1
        
        # ETF 0
        self.ETF0 = CSTMC(self, 2, "ETF0")
        
        # ETF 0 trace capture
        self.ETF0Trace = TMCETBTraceCapture(self, self.ETF0, "ETF0")
        
        # ETR 0
        self.ETR0 = ETRTraceCapture(self, 11, "ETR0")
        
        # Funnel 0
        self.Funnel0 = self.createFunnel(16, "Funnel0")
        
    def exposeCores(self):
        for core in self.cortexA57cores:
            a57_rams.registerInternalRAMs(core)
            self.addDeviceInterface(self.createDAPWrapper(core))
    
    def setupETFTrace(self, etfTrace, name, traceComponentOrder, managedDevices):
        '''Setup ETF trace capture'''
        # Use continuous mode
        etfTrace.setFormatterMode(FormatterMode.CONTINUOUS)
        
        # Register other trace components with ETF and register ETF with configuration
        etfTrace.setTraceComponentOrder(traceComponentOrder)
        self.addTraceCaptureInterface(etfTrace)
        
        # Automatically handle connection/disconnection to trace components
        self.addManagedTraceDevices(name, managedDevices)
    
    def setupETRTrace(self, etr, name, traceComponentOrder, managedDevices):
        '''Setup ETR trace capture'''
        # Use continuous mode
        etr.setFormatterMode(FormatterMode.CONTINUOUS)
        
        # Register other trace components with ETR and register ETR with configuration
        etr.setTraceComponentOrder(traceComponentOrder)
        self.addTraceCaptureInterface(etr)
        
        # Automatically handle connection/disconnection to trace components
        self.addManagedTraceDevices(name, managedDevices)
    
    def getCTIInfoForCore(self, core):
        '''Get the CTI info associated with a core
        return None if no associated CTI info
        '''
        
        # Build map of cores to DeviceCTIInfo objects
        ctiInfoMap = {}
        ctiInfoMap[self.cortexA57cores[0]] = CTISyncSMPDevice.DeviceCTIInfo(self.CoreCTIs[0], CTISyncSMPDevice.DeviceCTIInfo.NONE, 1, 0, 0)
        ctiInfoMap[self.cortexA57cores[1]] = CTISyncSMPDevice.DeviceCTIInfo(self.CoreCTIs[1], CTISyncSMPDevice.DeviceCTIInfo.NONE, 1, 0, 0)
        
        return ctiInfoMap.get(core, None)
    
    def getCTIForSource(self, source):
        '''Get the CTI and input/channel associated with a trace source
        return (None, None, None) if no associated CTI
        '''
        
        # Build map of trace sources to CTIs
        sourceCTIMap = {}
        sourceCTIMap[self.ETMs[0]] = (self.CoreCTIs[0], 6, CTM_CHANNEL_TRACE_TRIGGER)
        sourceCTIMap[self.ETMs[1]] = (self.CoreCTIs[1], 6, CTM_CHANNEL_TRACE_TRIGGER)
        
        return sourceCTIMap.get(source, (None, None, None))
    
    def getCTIForSink(self, sink):
        '''Get the CTI and output/channel associated with a trace sink
        return (None, None, None) if no associated CTI
        '''
        
        # Build map of trace sinks to CTIs
        sinkCTIMap = {}
        
        return sinkCTIMap.get(sink, (None, None, None))
    
    def getTMForCore(self, core):
        '''Get trace macrocell for core'''
        
        # Build map of cores to trace macrocells
        coreTMMap = {}
        coreTMMap[self.cortexA57cores[0]] = self.ETMs[0]
        coreTMMap[self.cortexA57cores[1]] = self.ETMs[1]
        
        return coreTMMap.get(core, None)
    
    def setTraceSourceEnabled(self, source, enabled):
        '''Enable/disable a trace source'''
        source.setEnabled(enabled)
        self.enableFunnelPortForSource(source, enabled)
        self.enableCTIsForSource(source, enabled)
    
    def createETM(self, etmDev, streamID, name):
        '''Create ETM of correct version'''
        if etmDev == 7:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
        if etmDev == 18:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
    
    def setupCTISyncSMP(self):
        '''Create SMP device using CTI synchronization'''
        
        # Setup CTIs for sync start/stop
        # Cortex-A57 CTI SMP setup
        ctiInfo = {}
        for c in self.cortexA57cores:
            ctiInfo[c] = self.getCTIInfoForCore(c)
        smp = CTISyncSMPDevice(self, "Cortex-A57 SMP", self.cortexA57cores, ctiInfo, CTM_CHANNEL_SYNC_START, CTM_CHANNEL_SYNC_STOP)
        self.addDeviceInterface(self.createDAPWrapper(smp))
        
        # Automatically handle connection to CTIs
        self.addManagedPlatformDevices(self.CoreCTIs)
    
    def setETFTraceEnabled(self, etfTrace, enabled):
        '''Enable/disable ETF trace capture'''
        if enabled:
            # Put the ETF in ETB mode
            etfTrace.getTMC().setMode(CSTMC.Mode.ETB)
        else:
            # Put the ETF in FIFO mode
            etfTrace.getTMC().setMode(CSTMC.Mode.ETF)
        
        self.enableCTIsForSink(etfTrace, enabled)
    
    def setETRTraceEnabled(self, etr, enabled):
        '''Enable/disable ETR trace capture'''
        self.enableCTIsForSink(etr, enabled)
    
    def registerTraceSources(self, traceCapture):
        '''Register all trace sources with trace capture device'''
        for c in range(0, NUM_CORES_CORTEX_A57):
            coreTM = self.getTMForCore(self.cortexA57cores[c])
            if coreTM.isEnabled():
                self.registerCoreTraceSource(traceCapture, self.cortexA57cores[c], coreTM)
        
        self.registerTraceSource(traceCapture, self.STM0)
        self.registerTraceSource(traceCapture, self.STM1)
    
    def registerCoreTraceSource(self, traceCapture, core, source):
        '''Register a trace source with trace capture device and enable triggers'''
        # Register with trace capture, associating with core
        traceCapture.addTraceSource(source, core.getID())
        
        # Source is managed by the configuration
        self.addManagedTraceDevices(traceCapture.getName(), [ source ])
        
        # CTI (if present) is also managed by the configuration
        cti, input, channel = self.getCTIForSource(source)
        if cti:
            self.addManagedTraceDevices(traceCapture.getName(), [ cti ])
    
    def getFunnelPortForSource(self, source):
        '''Get the funnel port number for a trace source'''
        
        # Build map of sources to funnels and funnel ports
        funnelMap = {}
        funnelMap[self.STM0] = (self.Funnel0, 3)
        funnelMap[self.STM1] = (self.Funnel0, 3)
        funnelMap[self.ETMs[0]] = (self.Funnel0, 0)
        funnelMap[self.ETMs[1]] = (self.Funnel0, 1)
        
        return funnelMap.get(source, (None, None))
    
    # +--------------------------------+
    # | Callback functions for options |
    # +--------------------------------+
    
    def optionValuesChanged(self):
        '''Callback to update the configuration state after options are changed'''
        if not self.isConnected():
            self.setInitialOptions()
        self.updateDynamicOptions()
        
    def setInitialOptions(self):
        '''Set the initial options'''
        
        traceMode = self.getOptionValue("options.trace.traceCapture")
        
        coreTraceEnabled = self.getOptionValue("options.cortexA57.coreTrace")
        for i in range(0, NUM_CORES_CORTEX_A57):
            thisCoreTraceEnabled = self.getOptionValue("options.cortexA57.coreTrace.Cortex_A57_%d" % i)
            enableSource = coreTraceEnabled and thisCoreTraceEnabled
            coreTM = self.getTMForCore(self.cortexA57cores[i])
            self.setTraceSourceEnabled(coreTM, enableSource)
            self.setInternalTraceRange(coreTM, "cortexA57")
        
        stmEnabled = self.getOptionValue("options.stm.STM0")
        self.setTraceSourceEnabled(self.STM0, stmEnabled)
        
        stmEnabled = self.getOptionValue("options.stm.STM1")
        self.setTraceSourceEnabled(self.STM1, stmEnabled)
        
        # Register trace sources for each trace sink
        self.registerTraceSources(self.ETF0Trace)
        self.registerTraceSources(self.ETR0)
        
        self.setManagedDeviceList(self.getManagedDevices(traceMode))
        
    def updateDynamicOptions(self):
        '''Update the dynamic options'''
        
        # Set up the ETR 0 buffer
        configureETRBuffer = self.getOptionValue("options.ETR.etrBuffer0")
        if configureETRBuffer:
            scatterGatherMode = self.getOptionValue("options.ETR.etrBuffer0.scatterGather")
            bufferStart = self.getOptionValue("options.ETR.etrBuffer0.start")
            bufferSize = self.getOptionValue("options.ETR.etrBuffer0.size")
            self.ETR0.setBaseAddress(bufferStart)
            self.ETR0.setTraceBufferSize(bufferSize)
            self.ETR0.setScatterGatherModeEnabled(scatterGatherMode)
            
        for i in range(0, NUM_CORES_CORTEX_A57):
            a57_rams.applyCacheDebug(configuration = self,
                                     optionName = "options.rams.cacheDebug",
                                     device = self.cortexA57cores[i])
            a57_rams.applyCachePreservation(configuration = self,
                                            optionName = "options.rams.cachePreserve",
                                            device = self.cortexA57cores[i])
        
    def getManagedDevices(self, traceKey):
        '''Get the required set of managed devices for this configuration'''
        deviceList = self.mgdPlatformDevs[:]
        for d in self.mgdTraceDevs.get(traceKey, []):
            if d not in deviceList:
                deviceList.append(d)
        
        return deviceList
    
    def setTraceCaptureMethod(self, method):
        if method == "none":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETF0":
            self.setETFTraceEnabled(self.ETF0Trace, True)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETR0":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETRTraceEnabled(self.ETR0, True)
    
    def getETMs(self):
        '''Get the ETMs'''
        return self.ETMs
    
    def getETMsForCortex_A57(self):
        '''Get the ETMs for Cortex-A57 cores only'''
        return [self.getTMForCore(core) for core in self.cortexA57cores]
    
    # +------------------------------+
    # | Target independent functions |
    # +------------------------------+
    
    def addManagedPlatformDevices(self, devs):
        '''Add devices to the list of devices managed by the configuration, as long as they are not already present'''
        for d in devs:
            if d not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(d)
    
    def registerTraceSource(self, traceCapture, source):
        '''Register trace source with trace capture device'''
        traceCapture.addTraceSource(source)
        self.addManagedTraceDevices(traceCapture.getName(), [ source ])
    
    def addManagedTraceDevices(self, traceKey, devs):
        '''Add devices to the set of devices managed by the configuration for this trace mode'''
        traceDevs = self.mgdTraceDevs.get(traceKey)
        if not traceDevs:
            traceDevs = []
            self.mgdTraceDevs[traceKey] = traceDevs
        for d in devs:
            if d not in traceDevs:
                traceDevs.append(d)
    
    def createDAPWrapper(self, core):
        '''Add a wrapper around a core to allow access to AHB and APB via the DAP'''
        return MemoryRouter(
            [DapMemoryAccessor("AHB", self.dap, 0, "AHB bus accessed via AP_0 on DAP_0"),
             DapMemoryAccessor("APB", self.dap, 1, "APB bus accessed via AP_1 on DAP_0")],
            core)
    
    def setInternalTraceRange(self, coreTM, coreName):
        
        traceRangeEnable = self.getOptionValue("options.%s.coreTrace.traceRange" % coreName)
        traceRangeStart = self.getOptionValue("options.%s.coreTrace.traceRange.start" % coreName)
        traceRangeEnd = self.getOptionValue("options.%s.coreTrace.traceRange.end" % coreName)
        
        if coreTM in self.traceRangeIDs:
            coreTM.clearTraceRange(self.traceRangeIDs[coreTM])
            del self.traceRangeIDs[coreTM]
        
        if traceRangeEnable:
            self.traceRangeIDs[coreTM] = coreTM.addTraceRange(traceRangeStart, traceRangeEnd)
    
    def enableCTIsForSource(self, source, enabled):
        '''Enable/disable triggers using CTI associated with source'''
        cti, input, channel = self.getCTIForSource(source)
        if cti:
            self.enableCTIInput(cti, input, channel, enabled)
    
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
    
    def enableFunnelPortForSource(self, source, enabled):
        '''Enable/disable the funnel port for a trace source'''
        funnel, port = self.getFunnelPortForSource(source)
        if funnel:
            if enabled:
                funnel.setPortEnabled(port)
            else:
                funnel.setPortDisabled(port)
    
    def createSTM(self, stmDev, streamID, name):
        stm = STMTraceSource(self, stmDev, streamID, name)
        # Disabled by default - will enable with option
        stm.setEnabled(False)
        return stm
    

