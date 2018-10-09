from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import FormatterMode
from com.arm.debug.dtsl.components import AXIAP
from com.arm.debug.dtsl.components import AXIMemAPAccessor
from com.arm.debug.dtsl.components import APBAP
from com.arm.debug.dtsl.components import AxBMemAPAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.configurations.options import IIntegerOption
from com.arm.debug.dtsl.components import CSTMC
from com.arm.debug.dtsl.components import TMCETBTraceCapture
from com.arm.debug.dtsl.components import ETRTraceCapture
from com.arm.debug.dtsl.components import CSCTI
from com.arm.debug.dtsl.components import ETMv4TraceSource
from com.arm.debug.dtsl.components import CSFunnel
from com.arm.debug.dtsl.components import STMTraceSource
from com.arm.debug.dtsl.components import SimpleSyncSMPDevice
from com.arm.debug.dtsl.configurations import TimestampInfo
from com.arm.debug.dtsl.interfaces import IARMCoreTraceSource

NUM_CORES_CORTEX_A53 = 4
CTM_CHANNEL_SYNC_STOP = 2  # Use channel 2 for sync stop
CTM_CHANNEL_SYNC_START = 1  # Use channel 1 for sync start
CTM_CHANNEL_TRACE_TRIGGER = 3  # Use channel 3 for trace triggers

# Import core specific functions
import a53_rams


class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=[
                DTSLv1.tabPage("trace", "Trace Capture", childOptions=[
                    DTSLv1.enumOption('traceCapture', 'Trace capture method', defaultValue="none",
                        values = [("none", "None"), ("ETF0", "On Chip Trace Buffer (ETF0/TMC)"), ("ETF1", "On Chip Trace Buffer (ETF1/TMC)"), ("ETF2", "On Chip Trace Buffer (ETF2/TMC)"), ("ETF3", "On Chip Trace Buffer (ETF3/TMC)"), ("ETR0", "System Memory Trace Buffer (ETR0/TMC)")],
                        setter=DtslScript.setTraceCaptureMethod),
                    DTSLv1.integerOption('timestampFrequency', 'Timestamp frequency', defaultValue=25000000, isDynamic=False, description="This value will be used to set the Counter Base Frequency ID Register of the Timestamp generator.\nIt represents the number of ticks per second and is used to translate the timestamp value reported into a number of seconds.\nNote that changing this value may not result in a change in the observed frequency."),
                ]),
                DTSLv1.tabPage("cortexA53", "Cortex-A53", childOptions=[
                    DTSLv1.booleanOption('coreTrace', 'Enable Cortex-A53 core trace', defaultValue=False,
                        childOptions =
                            # Allow each source to be enabled/disabled individually
                            [ DTSLv1.booleanOption('Cortex_A53_%d' % c, "Enable Cortex-A53 %d trace" % c, defaultValue=True)
                            for c in range(0, NUM_CORES_CORTEX_A53) ] +
                            [ DTSLv1.booleanOption('timestamp', "Enable ETM Timestamps", description="Controls the output of timestamps into the ETM output streams", defaultValue=True) ] +
                            [ DTSLv1.booleanOption('contextIDs', "Enable ETM Context IDs", description="Controls the output of context ID values into the ETM output streams", defaultValue=True,
                                childOptions = [
                                    DTSLv1.enumOption('contextIDsSize', 'Context ID Size', defaultValue="32",
                                        values = [("8", "8 bit"), ("16", "16 bit"), ("32", "32 bit")])
                                    ]),
                            ] +
                            [ ETMv4TraceSource.cycleAccurateOption(DtslScript.getETMsForCortex_A53) ]
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
        
        # Only MEM_AP devices are managed by default - others will be added when enabling trace, SMP etc
        if self.AXI not in self.mgdPlatformDevs:
            self.mgdPlatformDevs.append(self.AXI)
        if self.APB not in self.mgdPlatformDevs:
            self.mgdPlatformDevs.append(self.APB)
        
        self.exposeCores()
        
        traceComponentOrder = [ self.Funnel6 ]
        managedDevices = [ self.Funnel6, self.ETF0Trace ]
        self.setupETFTrace(self.ETF0Trace, "ETF0", traceComponentOrder, managedDevices)
        
        traceComponentOrder = []
        managedDevices = [ self.ETF1Trace ]
        self.setupETFTrace(self.ETF1Trace, "ETF1", traceComponentOrder, managedDevices)
        
        traceComponentOrder = []
        managedDevices = [ self.ETF2Trace ]
        self.setupETFTrace(self.ETF2Trace, "ETF2", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel6, self.ETF0, self.Funnel0, self.Funnel5 ]
        managedDevices = [ self.Funnel6, self.ETF0, self.Funnel0, self.Funnel5, self.ETF3Trace ]
        self.setupETFTrace(self.ETF3Trace, "ETF3", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel6, self.ETF0, self.Funnel0, self.Funnel5, self.ETF3 ]
        managedDevices = [ self.Funnel6, self.ETF0, self.Funnel0, self.Funnel5, self.ETF3, self.ETR0 ]
        self.setupETRTrace(self.ETR0, "ETR0", traceComponentOrder, managedDevices)
        
        self.setupSimpleSyncSMP()
        
        self.setManagedDeviceList(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''Find and create devices'''
        
        memApDev = 0
        
        memApDev = self.findDevice("CSMEMAP", memApDev + 1)
        self.AXI = AXIAP(self, memApDev, "AXI")
        
        memApDev = self.findDevice("CSMEMAP", memApDev + 1)
        self.APB = APBAP(self, memApDev, "APB")
        
        cortexA53coreDevs = [22, 26, 30, 34]
        self.cortexA53cores = []
        
        streamID = 1
        
        coreCTIDevs = [23, 27, 31, 35]
        self.CoreCTIs = []
        
        etmDevs = [25, 29, 33, 37]
        self.ETMs = []
        
        # STM 0
        self.STM0 = self.createSTM(13, streamID, "STM0")
        streamID += 1
        
        # STM 1
        self.STM1 = self.createSTM(16, streamID, "STM1")
        streamID += 1
        
        for i in range(0, NUM_CORES_CORTEX_A53):
            # Create core
            core = a53_rams.A53CoreDevice(self, cortexA53coreDevs[i], "Cortex-A53_%d" % i)
            self.cortexA53cores.append(core)
            
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
        self.ETF0 = CSTMC(self, 10, "ETF0")
        
        # ETF 0 trace capture
        self.ETF0Trace = TMCETBTraceCapture(self, self.ETF0, "ETF0")
        
        # ETF 1
        self.ETF1 = CSTMC(self, 12, "ETF1")
        
        # ETF 1 trace capture
        self.ETF1Trace = TMCETBTraceCapture(self, self.ETF1, "ETF1")
        
        # ETF 2
        self.ETF2 = CSTMC(self, 15, "ETF2")
        
        # ETF 2 trace capture
        self.ETF2Trace = TMCETBTraceCapture(self, self.ETF2, "ETF2")
        
        # ETF 3
        self.ETF3 = CSTMC(self, 18, "ETF3")
        
        # ETF 3 trace capture
        self.ETF3Trace = TMCETBTraceCapture(self, self.ETF3, "ETF3")
        
        # ETR 0
        self.ETR0 = ETRTraceCapture(self, 19, "ETR0")
        
        # Funnel 0
        self.Funnel0 = self.createFunnel(4, "Funnel0")
        # self.ETF0 is connected to self.Funnel0 port 0
        self.Funnel0.setPortEnabled(0)
        
        # Funnel 1
        self.Funnel1 = self.createFunnel(5, "Funnel1")
        
        # Funnel 2
        self.Funnel2 = self.createFunnel(6, "Funnel2")
        
        # Funnel 3
        self.Funnel3 = self.createFunnel(7, "Funnel3")
        
        # Funnel 4
        self.Funnel4 = self.createFunnel(8, "Funnel4")
        
        # Funnel 5
        self.Funnel5 = self.createFunnel(9, "Funnel5")
        # self.Funnel0 is connected to self.Funnel5 port 0
        self.Funnel5.setPortEnabled(0)
        
        # Funnel 6
        self.Funnel6 = self.createFunnel(21, "Funnel6")
        
    def registerFilters(self, core):
        '''Register MemAP filters to allow access to the AHB/APB for the device'''
        core.registerAddressFilters([
            AXIMemAPAccessor("AXI", self.AXI, "AXI bus accessed via AP", 64),
            AxBMemAPAccessor("APB", self.APB, "APB bus accessed via AP"),
        ])
    
    def exposeCores(self):
        for core in self.cortexA53cores:
            a53_rams.registerInternalRAMs(core)
            self.registerFilters(core)
            self.addDeviceInterface(core)
    
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
    
    def getCTIForSource(self, source):
        '''Get the CTI and input/channel associated with a trace source
        return (None, None, None) if no associated CTI
        '''
        
        # Build map of trace sources to CTIs
        sourceCTIMap = {}
        sourceCTIMap[self.ETMs[0]] = (self.CoreCTIs[0], 6, CTM_CHANNEL_TRACE_TRIGGER)
        sourceCTIMap[self.ETMs[1]] = (self.CoreCTIs[1], 6, CTM_CHANNEL_TRACE_TRIGGER)
        sourceCTIMap[self.ETMs[2]] = (self.CoreCTIs[2], 6, CTM_CHANNEL_TRACE_TRIGGER)
        sourceCTIMap[self.ETMs[3]] = (self.CoreCTIs[3], 6, CTM_CHANNEL_TRACE_TRIGGER)
        
        return sourceCTIMap.get(source, (None, None, None))
    
    def getTMForCore(self, core):
        '''Get trace macrocell for core'''
        
        # Build map of cores to trace macrocells
        coreTMMap = {}
        coreTMMap[self.cortexA53cores[0]] = self.ETMs[0]
        coreTMMap[self.cortexA53cores[1]] = self.ETMs[1]
        coreTMMap[self.cortexA53cores[2]] = self.ETMs[2]
        coreTMMap[self.cortexA53cores[3]] = self.ETMs[3]
        
        return coreTMMap.get(core, None)
    
    def setTraceSourceEnabled(self, source, enabled):
        '''Enable/disable a trace source'''
        source.setEnabled(enabled)
        self.enableFunnelPortForSource(source, enabled)
        self.enableCTIsForSource(source, enabled)
    
    def createETM(self, etmDev, streamID, name):
        '''Create ETM of correct version'''
        if etmDev == 25:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
        if etmDev == 29:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
        if etmDev == 33:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
        if etmDev == 37:
            etm = ETMv4TraceSource(self, etmDev, streamID, name)
            # Disabled by default - will enable with option
            etm.setEnabled(False)
            self.ETMs.append(etm)
            return etm
    
    def setupSimpleSyncSMP(self):
        '''Create SMP device using RDDI synchronization'''
        
        # Create SMP device and expose from configuration
        # Cortex-A53x4 SMP
        smp = SimpleSyncSMPDevice(self, "Cortex-A53 SMP", self.cortexA53cores)
        self.registerFilters(smp)
        self.addDeviceInterface(smp)
    
    def setETFTraceEnabled(self, etfTrace, enabled):
        '''Enable/disable ETF trace capture'''
        if enabled:
            # Put the ETF in ETB mode
            etfTrace.getTMC().setMode(CSTMC.Mode.ETB)
        else:
            # Put the ETF in FIFO mode
            etfTrace.getTMC().setMode(CSTMC.Mode.ETF)
    
    def setETRTraceEnabled(self, etr, enabled):
        '''Enable/disable ETR trace capture'''
    
    def registerTraceSources(self, traceCapture):
        '''Register all trace sources with trace capture device'''
        for c in range(0, NUM_CORES_CORTEX_A53):
            coreTM = self.getTMForCore(self.cortexA53cores[c])
            if coreTM.isEnabled():
                self.registerCoreTraceSource(traceCapture, self.cortexA53cores[c], coreTM)
        
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
        funnelMap[self.ETMs[0]] = (self.Funnel6, 0)
        funnelMap[self.ETMs[1]] = (self.Funnel6, 1)
        funnelMap[self.ETMs[2]] = (self.Funnel6, 2)
        funnelMap[self.ETMs[3]] = (self.Funnel6, 3)
        
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
        
        coreTraceEnabled = self.getOptionValue("options.cortexA53.coreTrace")
        for i in range(0, NUM_CORES_CORTEX_A53):
            thisCoreTraceEnabled = self.getOptionValue("options.cortexA53.coreTrace.Cortex_A53_%d" % i)
            enableSource = coreTraceEnabled and thisCoreTraceEnabled
            coreTM = self.getTMForCore(self.cortexA53cores[i])
            self.setTraceSourceEnabled(coreTM, enableSource)
            self.setTimestampingEnabled(coreTM, self.getOptionValue("options.cortexA53.coreTrace.timestamp"))
            self.setContextIDEnabled(coreTM,
                                     self.getOptionValue("options.cortexA53.coreTrace.contextIDs"),
                                     self.getOptionValue("options.cortexA53.coreTrace.contextIDs.contextIDsSize"))
        
        stmEnabled = self.getOptionValue("options.stm.STM0")
        self.setTraceSourceEnabled(self.STM0, stmEnabled)
        
        stmEnabled = self.getOptionValue("options.stm.STM1")
        self.setTraceSourceEnabled(self.STM1, stmEnabled)
        
        # Register trace sources for each trace sink
        self.registerTraceSources(self.ETF0Trace)
        self.registerTraceSources(self.ETF1Trace)
        self.registerTraceSources(self.ETF2Trace)
        self.registerTraceSources(self.ETF3Trace)
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
            
        for i in range(0, NUM_CORES_CORTEX_A53):
            a53_rams.applyCacheDebug(configuration = self,
                                     optionName = "options.rams.cacheDebug",
                                     device = self.cortexA53cores[i])
            a53_rams.applyCachePreservation(configuration = self,
                                            optionName = "options.rams.cachePreserve",
                                            device = self.cortexA53cores[i])
        
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
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETF0":
            self.setETFTraceEnabled(self.ETF0Trace, True)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETF1":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, True)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETF2":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, True)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETF3":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, True)
            self.setETRTraceEnabled(self.ETR0, False)
        elif method == "ETR0":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETRTraceEnabled(self.ETR0, True)
    
    def getETMs(self):
        '''Get the ETMs'''
        return self.ETMs
    
    def getETMsForCortex_A53(self):
        '''Get the ETMs for Cortex-A53 cores only'''
        return [self.getTMForCore(core) for core in self.cortexA53cores]
    
    # +------------------------------+
    # | Target independent functions |
    # +------------------------------+
    
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
    
    def postConnect(self):
        DTSLv1.postConnect(self)
        
        freq = self.getOptionValue("options.trace.timestampFrequency")
        
        # Update the value so the trace decoder can access it
        tsInfo = TimestampInfo(freq)
        self.setTimestampInfo(tsInfo)
    
    def setTimestampingEnabled(self, xtm, state):
        xtm.setTimestampingEnabled(state)
    
    def setContextIDEnabled(self, xtm, state, size):
        if state == False:
            xtm.setContextIDs(False, IARMCoreTraceSource.ContextIDSize.NONE)
        else:
            contextIDSizeMap = {
                 "8":IARMCoreTraceSource.ContextIDSize.BITS_7_0,
                "16":IARMCoreTraceSource.ContextIDSize.BITS_15_0,
                "32":IARMCoreTraceSource.ContextIDSize.BITS_31_0 }
            xtm.setContextIDs(True, contextIDSizeMap[size])
    

