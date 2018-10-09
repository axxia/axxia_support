from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import FormatterMode
from com.arm.debug.dtsl.components import AHBAP
from com.arm.debug.dtsl.components import AHBMemAPAccessor
from com.arm.debug.dtsl.components import APBAP
from com.arm.debug.dtsl.components import AxBMemAPAccessor
from com.arm.debug.dtsl.components import AXIAP
from com.arm.debug.dtsl.components import AXIMemAPAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.configurations.options import IIntegerOption
from com.arm.debug.dtsl.components import CSTMC
from com.arm.debug.dtsl.components import TMCETBTraceCapture
from com.arm.debug.dtsl.components import ETRTraceCapture
from com.arm.debug.dtsl.components import DSTREAMTraceCapture
from com.arm.debug.dtsl.components import CSCTI
from com.arm.debug.dtsl.components import ETMv4TraceSource
from com.arm.debug.dtsl.components import CSTPIU
from com.arm.debug.dtsl.components import CSFunnel
from com.arm.debug.dtsl.components import STMTraceSource
from com.arm.debug.dtsl.components import CTISyncSMPDevice
from com.arm.debug.dtsl.configurations import TimestampInfo
from com.arm.debug.dtsl.interfaces import IARMCoreTraceSource

tmDevs_cortexA57 = [["CSETM_0", "CSETM_1", "CSETM_2", "CSETM_3"],["CSETM_4", "CSETM_5", "CSETM_6", "CSETM_7"],["CSETM_8", "CSETM_9", "CSETM_10", "CSETM_11"],["CSETM_12", "CSETM_13", "CSETM_14", "CSETM_15"]]
ctiDevs_cortexA57 = [["CSCTI_7", "CSCTI_8", "CSCTI_9", "CSCTI_10"],["CSCTI_11", "CSCTI_12", "CSCTI_13", "CSCTI_14"],["CSCTI_15", "CSCTI_16", "CSCTI_17", "CSCTI_18"],["CSCTI_19", "CSCTI_20", "CSCTI_21", "CSCTI_22"]]
clusterDeviceNames_cortexA57 = ["Cortex-A57_SMP_0", "Cortex-A57_SMP_1", "Cortex-A57_SMP_2", "Cortex-A57_SMP_3"]
coreDevs_cortexA57 = [["Cortex-A57_0", "Cortex-A57_1", "Cortex-A57_2", "Cortex-A57_3"],["Cortex-A57_4", "Cortex-A57_5", "Cortex-A57_6", "Cortex-A57_7"],["Cortex-A57_8", "Cortex-A57_9", "Cortex-A57_10", "Cortex-A57_11"],["Cortex-A57_12", "Cortex-A57_13", "Cortex-A57_14", "Cortex-A57_15"]]
NUM_CLUSTERS_CORTEX_A57 = 4
NUM_CORES_CORTEX_A57_CLUSTERS = [4,4,4,4]
coresDap0 = ["Cortex-A57_0", "Cortex-A57_1", "Cortex-A57_2", "Cortex-A57_3", "Cortex-A57_4", "Cortex-A57_5", "Cortex-A57_6", "Cortex-A57_7", "Cortex-A57_8", "Cortex-A57_9", "Cortex-A57_10", "Cortex-A57_11", "Cortex-A57_12", "Cortex-A57_13", "Cortex-A57_14", "Cortex-A57_15"]
DSTREAM_PORTWIDTH = 16
CTM_CHANNEL_SYNC_STOP = 2  # Use channel 2 for sync stop
CTM_CHANNEL_SYNC_START = 1  # Use channel 1 for sync start
CTM_CHANNEL_TRACE_TRIGGER = 3  # Use channel 3 for trace triggers

# Import core specific functions
import a57_rams


class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=
                [DTSLv1.tabPage("trace", "Trace Capture", childOptions=[
                    DTSLv1.enumOption('traceCapture', 'Trace capture method', defaultValue="none",
                        values = [("none", "None"), ("CSTMC_0", "On Chip Trace Buffer (CSTMC_0/ETF)"), ("CSTMC_1", "On Chip Trace Buffer (CSTMC_1/ETF)"), ("CSTMC_2", "On Chip Trace Buffer (CSTMC_2/ETF)"), ("CSTMC_3", "On Chip Trace Buffer (CSTMC_3/ETF)"), ("CSTMC_4", "On Chip Trace Buffer (CSTMC_4/ETF)"), ("CSTMC_5", "On Chip Trace Buffer (CSTMC_5/ETF)"), ("CSTMC_6", "System Memory Trace Buffer (CSTMC_6/ETR)"), ("DSTREAM", "DSTREAM 4GB Trace Buffer")],
                        setter=DtslScript.setTraceCaptureMethod),
                    DTSLv1.integerOption('timestampFrequency', 'Timestamp frequency', defaultValue=25000000, isDynamic=False, description="This value will be used to set the Counter Base Frequency ID Register of the Timestamp generator.\nIt represents the number of ticks per second and is used to translate the timestamp value reported into a number of seconds.\nNote that changing this value may not result in a change in the observed frequency."),
                            DTSLv1.enumOption('tpiuPortWidth', 'TPIU Port Width', defaultValue="16",
                                values = [("4", "4 bit"), ("8", "8 bit"), ("16", "16 bit")], isDynamic=False)
                ])]
                +[DTSLv1.tabPage("cortexA57Cluster%d" % cluster, "Cortex-A57 Cluster %d" %cluster, childOptions=[
                    DTSLv1.booleanOption('coreTrace', 'Enable Cortex-A57 cluster %d trace' % cluster, defaultValue=False,
                        childOptions =
                            # Allow each source to be enabled/disabled individually
                            [ DTSLv1.booleanOption('Cortex_A57_%d_%d' % (cluster,core), "Enable " + coreDevs_cortexA57[cluster][core] + "trace", defaultValue=True)
                            for core in range(NUM_CORES_CORTEX_A57_CLUSTERS[cluster]) ] +
                            [ DTSLv1.booleanOption('timestamp', "Enable ETM Timestamps", description="Controls the output of timestamps into the ETM output streams", defaultValue=True) ] +
                            [ DTSLv1.booleanOption('contextIDs', "Enable ETM Context IDs", description="Controls the output of context ID values into the ETM output streams", defaultValue=True,
                                childOptions = [
                                    DTSLv1.enumOption('contextIDsSize', 'Context ID Size', defaultValue="32",
                                        values = [("8", "8 bit"), ("16", "16 bit"), ("32", "32 bit")])
                                    ]),
                            ] +
                            [ ETMv4TraceSource.cycleAccurateOption(DtslScript.getTraceMacrocellsForCoreTypeCluster("cortexA57", cluster))]
                        ),
                ])  for cluster in range(NUM_CLUSTERS_CORTEX_A57)]
                +[DTSLv1.tabPage("ETR", "ETR", childOptions=[
                    DTSLv1.booleanOption('etrBuffer0', 'Configure the system memory trace buffer to be used by the CSTMC_6/ETR device', defaultValue=False,
                        childOptions = [
                            DTSLv1.integerOption('start', 'Start address',
                            description='Start address of the system memory trace buffer to be used by the CSTMC_6/ETR device',
                            defaultValue=0x00100000,
                            display=IIntegerOption.DisplayFormat.HEX),
                            DTSLv1.integerOption('size', 'Size in bytes',
                            description='Size of the system memory trace buffer in bytes',
                            defaultValue=0x8000,
                            isDynamic=True,
                            display=IIntegerOption.DisplayFormat.HEX),
                            DTSLv1.booleanOption('scatterGather', 'Enable scatter-gather mode',
                            defaultValue=False,
                            description='When enabling scatter-gather mode, the start address of the on-chip trace buffer must point to a configured scatter-gather table')
                        ]
                    ),
                ])]
                +[DTSLv1.tabPage("stm", "STM", childOptions=[
                    DTSLv1.booleanOption('CSSTM_0', 'Enable CSSTM_0 trace', defaultValue=False),
                    DTSLv1.booleanOption('CSSTM_1', 'Enable CSSTM_1 trace', defaultValue=False),
                ])]
                +[DTSLv1.tabPage("rams", "Cache RAMs", childOptions=[
                    # Turn cache debug mode on/off
                    DTSLv1.booleanOption('cacheDebug', 'Cache debug mode',
                                         description='Turning cache debug mode on enables reading the cache RAMs. Enabling it may adversely impact debug performance.',
                                         defaultValue=False, isDynamic=True),
                    DTSLv1.booleanOption('cachePreserve', 'Preserve cache contents in debug state',
                                         description='Preserve the contents of caches while the core is stopped.',
                                         defaultValue=False, isDynamic=True),
                ])]
            )
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
        for i in range(len(self.AHBs)):
            if self.AHBs[i] not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(self.AHBs[i])
        
        for i in range(len(self.APBs)):
            if self.APBs[i] not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(self.APBs[i])
        
        for i in range(len(self.AXIs)):
            if self.AXIs[i] not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(self.AXIs[i])
        
        self.exposeCores()
        
        traceComponentOrder = [ self.Funnel1 ]
        managedDevices = [ self.Funnel1, self.OutCTI0, self.TPIU, self.ETF0Trace ]
        self.setupETFTrace(self.ETF0Trace, "CSTMC_0", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel2 ]
        managedDevices = [ self.Funnel2, self.OutCTI1, self.TPIU, self.ETF1Trace ]
        self.setupETFTrace(self.ETF1Trace, "CSTMC_1", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel3 ]
        managedDevices = [ self.Funnel3, self.OutCTI2, self.TPIU, self.ETF2Trace ]
        self.setupETFTrace(self.ETF2Trace, "CSTMC_2", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.Funnel4 ]
        managedDevices = [ self.Funnel4, self.OutCTI3, self.TPIU, self.ETF3Trace ]
        self.setupETFTrace(self.ETF3Trace, "CSTMC_3", traceComponentOrder, managedDevices)
        
        traceComponentOrder = []
        managedDevices = [ self.OutCTI4, self.TPIU, self.ETF4Trace ]
        self.setupETFTrace(self.ETF4Trace, "CSTMC_4", traceComponentOrder, managedDevices)
        
        traceComponentOrder = []
        managedDevices = [ self.OutCTI5, self.TPIU, self.ETF5Trace ]
        self.setupETFTrace(self.ETF5Trace, "CSTMC_5", traceComponentOrder, managedDevices)
        
        traceComponentOrder = []
        managedDevices = [ self.OutCTI6, self.TPIU, self.ETR0 ]
        self.setupETRTrace(self.ETR0, "CSTMC_6", traceComponentOrder, managedDevices)
        
        traceComponentOrder = [ self.ETF4, self.ETF5, self.Funnel1, self.ETF0, self.Funnel2, self.ETF1, self.Funnel3, self.ETF2, self.Funnel4, self.ETF3, self.Funnel0, self.TPIU ]
        managedDevices = [ self.ETF4, self.ETF5, self.Funnel1, self.ETF0, self.Funnel2, self.ETF1, self.Funnel3, self.ETF2, self.Funnel4, self.ETF3, self.Funnel0, self.OutCTI6, self.TPIU, self.DSTREAM ]
        self.setupDSTREAMTrace(DSTREAM_PORTWIDTH, traceComponentOrder, managedDevices)
        
        self.setupCTISyncSMP()
        
        self.setManagedDeviceList(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''Find and create devices'''
        
        apDevs_AHBs = ["CSMEMAP_2"]
        self.AHBs = []
        
        apDevs_APBs = ["CSMEMAP_1"]
        self.APBs = []
        
        apDevs_AXIs = ["CSMEMAP_0"]
        self.AXIs = []
        
        for i in range(len(apDevs_AHBs)):
            apDevice = AHBAP(self, self.findDevice(apDevs_AHBs[i]), "AHB_%d" % i)
            self.AHBs.append(apDevice)
        
        for i in range(len(apDevs_APBs)):
            apDevice = APBAP(self, self.findDevice(apDevs_APBs[i]), "APB_%d" % i)
            self.APBs.append(apDevice)
        
        for i in range(len(apDevs_AXIs)):
            apDevice = AXIAP(self, self.findDevice(apDevs_AXIs[i]), "AXI_%d" % i)
            self.AXIs.append(apDevice)
        
        self.cortexA57cores = []
        self.clusterCores_cortexA57 = {}
        
        # Trace start/stop CTI 0
        self.OutCTI0 = CSCTI(self, self.findDevice("CSCTI_0"), "CSCTI_0")
        
        # Trace start/stop CTI 1
        self.OutCTI1 = CSCTI(self, self.findDevice("CSCTI_1"), "CSCTI_1")
        
        # Trace start/stop CTI 2
        self.OutCTI2 = CSCTI(self, self.findDevice("CSCTI_2"), "CSCTI_2")
        
        # Trace start/stop CTI 3
        self.OutCTI3 = CSCTI(self, self.findDevice("CSCTI_3"), "CSCTI_3")
        
        # Trace start/stop CTI 4
        self.OutCTI4 = CSCTI(self, self.findDevice("CSCTI_4"), "CSCTI_4")
        
        # Trace start/stop CTI 5
        self.OutCTI5 = CSCTI(self, self.findDevice("CSCTI_5"), "CSCTI_5")
        
        # Trace start/stop CTI 6
        self.OutCTI6 = CSCTI(self, self.findDevice("CSCTI_6"), "CSCTI_6")
        
        self.CoreCTIs = []
        
        self.macrocells = {}
        self.macrocells["cortexA57"] = [ [] for i in range(NUM_CLUSTERS_CORTEX_A57) ]
        
        # The ATB stream ID which will be assigned to trace sources.
        streamID = 1
        
        # STM -- CSSTM_0
        self.STM0 = self.createSTM("CSSTM_0", streamID, "CSSTM_0")
        streamID += 1
        
        # STM -- CSSTM_1
        self.STM1 = self.createSTM("CSSTM_1", streamID, "CSSTM_1")
        streamID += 1
        
        for cluster in range(NUM_CLUSTERS_CORTEX_A57):
            #Ensure that any macrocell StreamIDs are grouped such that they can be filtered by a programmable replicator.
            streamID += (0x10 - (streamID % 0x10))
            self.clusterCores_cortexA57[cluster] = []
            for core in range(NUM_CORES_CORTEX_A57_CLUSTERS[cluster]):
                # Create core
                coreDevice = a57_rams.A57CoreDevice(self, self.findDevice(coreDevs_cortexA57[cluster][core]), coreDevs_cortexA57[cluster][core])
                self.cortexA57cores.append(coreDevice)
                self.clusterCores_cortexA57[cluster].append(coreDevice)
                
                # Create CTI (if a CTI exists for this core)
                if not ctiDevs_cortexA57[cluster][core] == None:
                    coreCTI = CSCTI(self, self.findDevice(ctiDevs_cortexA57[cluster][core]), ctiDevs_cortexA57[cluster][core])
                    self.CoreCTIs.append(coreCTI)
                
                # Create Trace Macrocell (if a macrocell exists for this core - disabled by default - will enable with option)
                if not tmDevs_cortexA57[cluster][core] == None:
                    tm =  ETMv4TraceSource(self, self.findDevice(tmDevs_cortexA57[cluster][core]), streamID, tmDevs_cortexA57[cluster][core])
                    streamID += 2
                    tm.setEnabled(False)
                    self.macrocells["cortexA57"][cluster].append(tm)
                
        # ETF 0
        self.ETF0 = CSTMC(self, self.findDevice("CSTMC_0"), "CSTMC_0")
        
        # ETF 0 trace capture
        self.ETF0Trace = TMCETBTraceCapture(self, self.ETF0, "CSTMC_0")
        
        # ETF 1
        self.ETF1 = CSTMC(self, self.findDevice("CSTMC_1"), "CSTMC_1")
        
        # ETF 1 trace capture
        self.ETF1Trace = TMCETBTraceCapture(self, self.ETF1, "CSTMC_1")
        
        # ETF 2
        self.ETF2 = CSTMC(self, self.findDevice("CSTMC_2"), "CSTMC_2")
        
        # ETF 2 trace capture
        self.ETF2Trace = TMCETBTraceCapture(self, self.ETF2, "CSTMC_2")
        
        # ETF 3
        self.ETF3 = CSTMC(self, self.findDevice("CSTMC_3"), "CSTMC_3")
        
        # ETF 3 trace capture
        self.ETF3Trace = TMCETBTraceCapture(self, self.ETF3, "CSTMC_3")
        
        # ETF 4
        self.ETF4 = CSTMC(self, self.findDevice("CSTMC_4"), "CSTMC_4")
        
        # ETF 4 trace capture
        self.ETF4Trace = TMCETBTraceCapture(self, self.ETF4, "CSTMC_4")
        
        # ETF 5
        self.ETF5 = CSTMC(self, self.findDevice("CSTMC_5"), "CSTMC_5")
        
        # ETF 5 trace capture
        self.ETF5Trace = TMCETBTraceCapture(self, self.ETF5, "CSTMC_5")
        
        # ETR 0
        self.ETR0 = ETRTraceCapture(self, self.findDevice("CSTMC_6"), "CSTMC_6")
        
        # DSTREAM
        self.createDSTREAM()
        
        # TPIU
        self.TPIU = self.createTPIU("CSTPIU", "TPIU")
        
        # Funnel 0
        self.Funnel0 = self.createFunnel("CSTFunnel_0", "CSTFunnel_0")
        # CSTMC_0 is connected to CSTFunnel_0 port 2
        self.Funnel0.setPortEnabled(2)
        # CSTMC_1 is connected to CSTFunnel_0 port 3
        self.Funnel0.setPortEnabled(3)
        # CSTMC_2 is connected to CSTFunnel_0 port 4
        self.Funnel0.setPortEnabled(4)
        # CSTMC_3 is connected to CSTFunnel_0 port 5
        self.Funnel0.setPortEnabled(5)
        # CSTMC_4 is connected to CSTFunnel_0 port 1
        self.Funnel0.setPortEnabled(1)
        # CSTMC_5 is connected to CSTFunnel_0 port 0
        self.Funnel0.setPortEnabled(0)
        
        # Funnel 1
        self.Funnel1 = self.createFunnel("CSTFunnel_1", "CSTFunnel_1")
        
        # Funnel 2
        self.Funnel2 = self.createFunnel("CSTFunnel_2", "CSTFunnel_2")
        
        # Funnel 3
        self.Funnel3 = self.createFunnel("CSTFunnel_3", "CSTFunnel_3")
        
        # Funnel 4
        self.Funnel4 = self.createFunnel("CSTFunnel_4", "CSTFunnel_4")
        
    def registerFilters(self, core, dap):
        '''Register MemAP filters to allow access to the APs for the device'''
        if dap == 0:
            core.registerAddressFilters([
                AHBMemAPAccessor("AHB_0", self.AHBs[0], "AHB bus accessed via AP 2 (CSMEMAP_2)"),
                AxBMemAPAccessor("APB_0", self.APBs[0], "APB bus accessed via AP 1 (CSMEMAP_1)"),
                AXIMemAPAccessor("AXI_0", self.AXIs[0], "AXI bus accessed via AP 0 (CSMEMAP_0)", 64),
            ])
    
    def exposeCores(self):
        for coreName in coresDap0:
            core = self.getDeviceInterface(coreName)
            self.registerFilters(core, 0)
            self.addDeviceInterface(core)
        for core in self.cortexA57cores:
            a57_rams.registerInternalRAMs(core)
    
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
    
    def setupDSTREAMTrace(self, portwidth, traceComponentOrder, managedDevices):
        '''Setup DSTREAM trace capture'''
        # Configure the TPIU mode
        self.TPIU.setFormatterMode(FormatterMode.CONTINUOUS)
        self.TPIU.setPortSize(portwidth)
        
        # Configure the DSTREAM for trace
        self.DSTREAM.setTraceMode(DSTREAMTraceCapture.TraceMode.Continuous)
        self.DSTREAM.setPortWidth(portwidth)
        
        # Register other trace components
        self.DSTREAM.setTraceComponentOrder(traceComponentOrder)
        
        # Register the DSTREAM with the configuration
        self.addTraceCaptureInterface(self.DSTREAM)
        
        # Automatically handle connection/disconnection to trace components
        self.addManagedTraceDevices("DSTREAM", managedDevices)
        
    
    def getFunnelPortForSource(self, source):
        '''Get the funnel port number for a trace source'''
        
        macrocellNames = ["CSSTM_0", "CSSTM_1", "CSETM_0", "CSETM_1", "CSETM_2", "CSETM_3", "CSETM_4", "CSETM_5", "CSETM_6", "CSETM_7", "CSETM_8", "CSETM_9", "CSETM_10", "CSETM_11", "CSETM_12", "CSETM_13", "CSETM_14", "CSETM_15"]
        funnelNames = [None, None, "CSTFunnel_1", "CSTFunnel_1", "CSTFunnel_1", "CSTFunnel_1", "CSTFunnel_2", "CSTFunnel_2", "CSTFunnel_2", "CSTFunnel_2", "CSTFunnel_3", "CSTFunnel_3", "CSTFunnel_3", "CSTFunnel_3", "CSTFunnel_4", "CSTFunnel_4", "CSTFunnel_4", "CSTFunnel_4"]
        funnelPorts = [None, None, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3]
        
        sourceName = source.getName()
        for i in range(len(macrocellNames)):
            if sourceName == macrocellNames[i]:
                return(self.getDeviceInterface(funnelNames[i]), funnelPorts[i])
        
        return (None, None)
    
    def getCTIInfoForCore(self, core):
        '''Get the funnel port number for a trace source'''
        
        coreNames = ["Cortex-A57_0", "Cortex-A57_1", "Cortex-A57_2", "Cortex-A57_3", "Cortex-A57_4", "Cortex-A57_5", "Cortex-A57_6", "Cortex-A57_7", "Cortex-A57_8", "Cortex-A57_9", "Cortex-A57_10", "Cortex-A57_11", "Cortex-A57_12", "Cortex-A57_13", "Cortex-A57_14", "Cortex-A57_15"]
        ctiNames = ["CSCTI_7", "CSCTI_8", "CSCTI_9", "CSCTI_10", "CSCTI_11", "CSCTI_12", "CSCTI_13", "CSCTI_14", "CSCTI_15", "CSCTI_16", "CSCTI_17", "CSCTI_18", "CSCTI_19", "CSCTI_20", "CSCTI_21", "CSCTI_22"]
        ctiTriggers = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
        
        coreName = core.getName()
        for i in range(len(coreNames)):
            if coreName == coreNames[i]:
                return CTISyncSMPDevice.DeviceCTIInfo(self.getDeviceInterface(ctiNames[i]), CTISyncSMPDevice.DeviceCTIInfo.NONE, ctiTriggers[i], 0, 0)
        
        return None
    
    def getCTIForSource(self, source):
        '''Get the CTI and input/channel associated with a trace source
        return (None, None, None) if no associated CTI
        '''
        macrocellNames = ["CSETM_0", "CSETM_1", "CSETM_2", "CSETM_3", "CSETM_4", "CSETM_5", "CSETM_6", "CSETM_7", "CSETM_8", "CSETM_9", "CSETM_10", "CSETM_11", "CSETM_12", "CSETM_13", "CSETM_14", "CSETM_15"]
        ctiNames = ["CSCTI_7", "CSCTI_8", "CSCTI_9", "CSCTI_10", "CSCTI_11", "CSCTI_12", "CSCTI_13", "CSCTI_14", "CSCTI_15", "CSCTI_16", "CSCTI_17", "CSCTI_18", "CSCTI_19", "CSCTI_20", "CSCTI_21", "CSCTI_22"]
        ctiTriggers = [6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6]
        
        sourceName = source.getName()
        for i in range(len(macrocellNames)):
            if sourceName == macrocellNames[i]:
                return (self.getDeviceInterface(ctiNames[i]), ctiTriggers[i], CTM_CHANNEL_TRACE_TRIGGER)
        
        return (None, None, None)
    
    def getCTIForSink(self, sink):
        '''Get the CTI and output/channel associated with a trace sink
        return (None, None, None) if no associated CTI
        '''
        sinkNames = ["CSTMC_0", "CSTMC_1", "CSTMC_2", "CSTMC_3", "CSTMC_4", "CSTMC_5", "CSTMC_6", "CSTPIU"]
        ctiNames = ["CSCTI_0", "CSCTI_1", "CSCTI_2", "CSCTI_3", "CSCTI_4", "CSCTI_5", "CSCTI_6", "CSCTI_6"]
        ctiTriggers = [1, 1, 1, 1, 1, 1, 1, 3]
        
        sinkName = sink.getName()
        for i in range(len(sinkNames)):
            if sinkName == sinkNames[i]:
                return (self.getDeviceInterface(ctiNames[i]), ctiTriggers[i], CTM_CHANNEL_TRACE_TRIGGER)
        
        return (None, None, None)
    
    def getTMForCore(self, core):
        '''Get trace macrocell for core'''
        coreNames = ["Cortex-A57_0", "Cortex-A57_1", "Cortex-A57_2", "Cortex-A57_3", "Cortex-A57_4", "Cortex-A57_5", "Cortex-A57_6", "Cortex-A57_7", "Cortex-A57_8", "Cortex-A57_9", "Cortex-A57_10", "Cortex-A57_11", "Cortex-A57_12", "Cortex-A57_13", "Cortex-A57_14", "Cortex-A57_15"]
        macrocellNames = ["CSETM_0", "CSETM_1", "CSETM_2", "CSETM_3", "CSETM_4", "CSETM_5", "CSETM_6", "CSETM_7", "CSETM_8", "CSETM_9", "CSETM_10", "CSETM_11", "CSETM_12", "CSETM_13", "CSETM_14", "CSETM_15"]
        
        coreName = core.getName()
        for i in range(len(coreNames)):
            if coreName == coreNames[i]:
                return self.getDeviceInterface(macrocellNames[i])
        
        return None
    
    def setTraceSourceEnabled(self, source, enabled):
        '''Enable/disable a trace source'''
        source.setEnabled(enabled)
        self.enableFunnelPortForSource(source, enabled)
        self.enableCTIsForSource(source, enabled)
    
    def createTPIU(self, tpiuDev, name):
        tpiu = CSTPIU(self, self.findDevice(tpiuDev), name)
        # Disabled by default - will enable with option
        tpiu.setEnabled(False)
        return tpiu
    
    def setupCTISyncSMP(self):
        '''Create SMP device using CTI synchronization'''
        
        # Setup CTIs for sync start/stop
        for cluster in range(NUM_CLUSTERS_CORTEX_A57):
            # SMP Device for this cluster
            ctiInfo = {}
            for c in self.clusterCores_cortexA57[cluster]:
                ctiInfo[c] = self.getCTIInfoForCore(c)
        
            smp = CTISyncSMPDevice(self, clusterDeviceNames_cortexA57[cluster], self.clusterCores_cortexA57[cluster], ctiInfo, CTM_CHANNEL_SYNC_START, CTM_CHANNEL_SYNC_STOP)
            self.registerFilters(smp, 0)
            self.addDeviceInterface(smp)
        
        # Cortex-A57x16 CTI SMP
        ctiInfo = {}
        for c in self.cortexA57cores:
            ctiInfo[c] = self.getCTIInfoForCore(c)
        
        smp = CTISyncSMPDevice(self, "Cortex-A57x16 SMP", self.cortexA57cores, ctiInfo, CTM_CHANNEL_SYNC_START, CTM_CHANNEL_SYNC_STOP)
        self.registerFilters(smp, 0)
        self.addDeviceInterface(smp)
        
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
        if enabled:
            # Ensure TPIU is disabled
            self.TPIU.setEnabled(False)
        self.enableCTIsForSink(etr, enabled)
    
    def setDSTREAMTraceEnabled(self, enabled):
        '''Enable/disable DSTREAM trace capture'''
        self.TPIU.setEnabled(enabled)
        self.enableCTIsForSink(self.DSTREAM, enabled)
    
    def registerTraceSources(self, traceCapture):
        '''Register all trace sources with trace capture device'''
        for core in self.cortexA57cores:
            coreTM = self.getTMForCore(core)
            if coreTM.isEnabled():
                self.registerCoreTraceSource(traceCapture, core, coreTM)
        
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
    
    # +--------------------------------+
    # | Callback functions for options |
    # +--------------------------------+
    
    def optionValuesChanged(self):
        '''Callback to update the configuration state after options are changed'''
        if not self.isConnected():
            try:
                self.setInitialOptions()
            except:
                pass
        self.updateDynamicOptions()
        
    def setInitialOptions(self):
        '''Set the initial options'''
        
        traceMode = self.getOptionValue("options.trace.traceCapture")
        
        i=0
        for cluster in range(NUM_CLUSTERS_CORTEX_A57):
            coreTraceEnabled = self.getOptionValue("options.cortexA57Cluster%d.coreTrace" % cluster)
            for core in range(NUM_CORES_CORTEX_A57_CLUSTERS[cluster]):
                thisCoreTraceEnabled = self.getOptionValue("options.cortexA57Cluster%d.coreTrace.Cortex_A57_%d_%d" % (cluster,cluster,core))
                enableSource = coreTraceEnabled and thisCoreTraceEnabled
                coreTM = self.getTMForCore(self.cortexA57cores[i])
                self.setTraceSourceEnabled(coreTM, enableSource)
                self.setTimestampingEnabled(coreTM, self.getOptionValue("options.cortexA57Cluster%d.coreTrace.timestamp" % cluster))
                self.setContextIDEnabled(coreTM,
                                         self.getOptionValue("options.cortexA57Cluster%d.coreTrace.contextIDs" % cluster),
                                         self.getOptionValue("options.cortexA57Cluster%d.coreTrace.contextIDs.contextIDsSize" % cluster))
                i+=1
            
        
        opt = self.getOptions().getOption("options.trace.tpiuPortWidth")
        if not opt is None:
            portWidth = self.getOptionValue("options.trace.tpiuPortWidth")
            self.TPIU.setPortSize(int(portWidth))
            self.DSTREAM.setPortWidth(int(portWidth))
        
        stmEnabled = self.getOptionValue("options.stm.CSSTM_0")
        self.setTraceSourceEnabled(self.STM0, stmEnabled)
        
        stmEnabled = self.getOptionValue("options.stm.CSSTM_1")
        self.setTraceSourceEnabled(self.STM1, stmEnabled)
        
        # Register trace sources for each trace sink
        self.registerTraceSources(self.ETF0Trace)
        self.registerTraceSources(self.ETF1Trace)
        self.registerTraceSources(self.ETF2Trace)
        self.registerTraceSources(self.ETF3Trace)
        self.registerTraceSources(self.ETF4Trace)
        self.registerTraceSources(self.ETF5Trace)
        self.registerTraceSources(self.ETR0)
        self.registerTraceSources(self.DSTREAM)
        
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
            
        for core in range(0, len(self.cortexA57cores)):
            a57_rams.applyCacheDebug(configuration = self,
                                     optionName = "options.rams.cacheDebug",
                                     device = self.cortexA57cores[core])
            a57_rams.applyCachePreservation(configuration = self,
                                            optionName = "options.rams.cachePreserve",
                                            device = self.cortexA57cores[core])
        
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
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_0":
            self.setETFTraceEnabled(self.ETF0Trace, True)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_1":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, True)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_2":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, True)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_3":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, True)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_4":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, True)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_5":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, True)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(False)
        elif method == "CSTMC_6":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, True)
            self.setDSTREAMTraceEnabled(False)
        elif method == "DSTREAM":
            self.setETFTraceEnabled(self.ETF0Trace, False)
            self.setETFTraceEnabled(self.ETF1Trace, False)
            self.setETFTraceEnabled(self.ETF2Trace, False)
            self.setETFTraceEnabled(self.ETF3Trace, False)
            self.setETFTraceEnabled(self.ETF4Trace, False)
            self.setETFTraceEnabled(self.ETF5Trace, False)
            self.setETRTraceEnabled(self.ETR0, False)
            self.setDSTREAMTraceEnabled(True)
    
    @staticmethod
    def getTraceMacrocellsForCoreTypeCluster(coreType, cluster):
        '''Get the Trace Macrocells for a given coreType
           Use parameter-binding to ensure that the correct Macrocells
           are returned for the core type and cluster passed only'''
        def getMacrocellsForCluster(self):
            return self.macrocells[coreType][cluster]
        return getMacrocellsForCluster
    
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
    
    def createDSTREAM(self):
        self.DSTREAM = DSTREAMTraceCapture(self, "DSTREAM")
    
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
        funnel = CSFunnel(self, self.findDevice(funnelDev), name)
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
        stm = STMTraceSource(self, self.findDevice(stmDev), streamID, name)
        # Disabled by default - will enable with option
        stm.setEnabled(False)
        return stm
    
    def postConnect(self):
        DTSLv1.postConnect(self)
        
        try:
            freq = self.getOptionValue("options.trace.timestampFrequency")
        except:
            return
        
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
    
