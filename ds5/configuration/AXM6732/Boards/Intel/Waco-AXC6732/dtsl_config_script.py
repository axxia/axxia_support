from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import APBAP
from com.arm.debug.dtsl.components import AxBMemAPAccessor
from com.arm.debug.dtsl.components import AXIAP
from com.arm.debug.dtsl.components import AXIMemAPAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.components import RDDISyncSMPDevice

clusterDeviceNames_cortexA53 = ["Cortex-A53_SMP_0", "Cortex-A53_SMP_1024", "Cortex-A53_SMP_1280", "Cortex-A53_SMP_1536", "Cortex-A53_SMP_1792", "Cortex-A53_SMP_256", "Cortex-A53_SMP_512", "Cortex-A53_SMP_768"]
coreDevs_cortexA53 = [["Cortex-A53_0", "Cortex-A53_1", "Cortex-A53_2", "Cortex-A53_3"],["Cortex-A53_16", "Cortex-A53_17", "Cortex-A53_18", "Cortex-A53_19"],["Cortex-A53_20", "Cortex-A53_21", "Cortex-A53_22", "Cortex-A53_23"],["Cortex-A53_24", "Cortex-A53_25", "Cortex-A53_26", "Cortex-A53_27"],["Cortex-A53_28", "Cortex-A53_29", "Cortex-A53_30", "Cortex-A53_31"],["Cortex-A53_4", "Cortex-A53_5", "Cortex-A53_6", "Cortex-A53_7"],["Cortex-A53_8", "Cortex-A53_9", "Cortex-A53_10", "Cortex-A53_11"],["Cortex-A53_12", "Cortex-A53_13", "Cortex-A53_14", "Cortex-A53_15"]]
NUM_CLUSTERS_CORTEX_A53 = 8
NUM_CORES_CORTEX_A53_CLUSTERS = [4,4,4,4,4,4,4,4]
coresDap0 = ["Cortex-A53_0", "Cortex-A53_1", "Cortex-A53_2", "Cortex-A53_3", "Cortex-A53_4", "Cortex-A53_5", "Cortex-A53_6", "Cortex-A53_7", "Cortex-A53_8", "Cortex-A53_9", "Cortex-A53_10", "Cortex-A53_11", "Cortex-A53_12", "Cortex-A53_13", "Cortex-A53_14", "Cortex-A53_15", "Cortex-A53_16", "Cortex-A53_17", "Cortex-A53_18", "Cortex-A53_19", "Cortex-A53_20", "Cortex-A53_21", "Cortex-A53_22", "Cortex-A53_23", "Cortex-A53_24", "Cortex-A53_25", "Cortex-A53_26", "Cortex-A53_27", "Cortex-A53_28", "Cortex-A53_29", "Cortex-A53_30", "Cortex-A53_31"]

# Import core specific functions
import a53_rams


class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=
                [DTSLv1.tabPage("rams", "Cache RAMs", childOptions=[
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
        
        # Locate devices on the platform and create corresponding objects
        self.discoverDevices()
        
        # Only MEM_AP devices are managed by default - others will be added when enabling trace, SMP etc
        for i in range(len(self.APBs)):
            if self.APBs[i] not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(self.APBs[i])
        
        for i in range(len(self.AXIs)):
            if self.AXIs[i] not in self.mgdPlatformDevs:
                self.mgdPlatformDevs.append(self.AXIs[i])
        
        self.exposeCores()
        
        self.setupSimpleSyncSMP()
        
        self.setManagedDeviceList(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''Find and create devices'''
        
        apDevs_APBs = ["CSMEMAP_1"]
        self.APBs = []
        
        apDevs_AXIs = ["CSMEMAP_0"]
        self.AXIs = []
        
        for i in range(len(apDevs_APBs)):
            apDevice = APBAP(self, self.findDevice(apDevs_APBs[i]), "APB_%d" % i)
            self.APBs.append(apDevice)
        
        for i in range(len(apDevs_AXIs)):
            apDevice = AXIAP(self, self.findDevice(apDevs_AXIs[i]), "AXI_%d" % i)
            self.AXIs.append(apDevice)
        
        self.cortexA53cores = []
        self.clusterCores_cortexA53 = {}
        
        for cluster in range(NUM_CLUSTERS_CORTEX_A53):
            self.clusterCores_cortexA53[cluster] = []
            for core in range(NUM_CORES_CORTEX_A53_CLUSTERS[cluster]):
                # Create core
                coreDevice = a53_rams.A53CoreDevice(self, self.findDevice(coreDevs_cortexA53[cluster][core]), coreDevs_cortexA53[cluster][core])
                self.cortexA53cores.append(coreDevice)
                self.clusterCores_cortexA53[cluster].append(coreDevice)
                
    def registerFilters(self, core, dap):
        '''Register MemAP filters to allow access to the APs for the device'''
        if dap == 0:
            core.registerAddressFilters([
                AxBMemAPAccessor("APB_0", self.APBs[0], "APB bus accessed via AP 1 (CSMEMAP_1)"),
                AXIMemAPAccessor("AXI_0", self.AXIs[0], "AXI bus accessed via AP 0 (CSMEMAP_0)", 64),
            ])
    
    def exposeCores(self):
        for coreName in coresDap0:
            core = self.getDeviceInterface(coreName)
            self.registerFilters(core, 0)
            self.addDeviceInterface(core)
        for core in self.cortexA53cores:
            a53_rams.registerInternalRAMs(core)
    
    def setupSimpleSyncSMP(self):
        '''Create SMP device(s) using RDDI synchronization'''
        
        for cluster in range(NUM_CLUSTERS_CORTEX_A53):
            # SMP Device for this cluster
            smp = RDDISyncSMPDevice(self, clusterDeviceNames_cortexA53[cluster], self.clusterCores_cortexA53[cluster])
            self.registerFilters(smp, 0)
            self.addDeviceInterface(smp)
        
        # Cortex-A53x32 SMP
        smp = RDDISyncSMPDevice(self, "Cortex-A53x32 SMP", self.cortexA53cores)
        self.registerFilters(smp, 0)
        self.addDeviceInterface(smp)
    
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
        
    def updateDynamicOptions(self):
        '''Update the dynamic options'''
        
        for core in range(0, len(self.cortexA53cores)):
            a53_rams.applyCacheDebug(configuration = self,
                                     optionName = "options.rams.cacheDebug",
                                     device = self.cortexA53cores[core])
            a53_rams.applyCachePreservation(configuration = self,
                                            optionName = "options.rams.cachePreserve",
                                            device = self.cortexA53cores[core])
        
