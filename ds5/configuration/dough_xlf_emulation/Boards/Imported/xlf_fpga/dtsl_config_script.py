from com.arm.debug.dtsl.configurations import DTSLv1
from com.arm.debug.dtsl.components import CSDAP
from com.arm.debug.dtsl.components import MemoryRouter
from com.arm.debug.dtsl.components import DapMemoryAccessor
from com.arm.debug.dtsl.components import Device
from com.arm.debug.dtsl.components import RDDISyncSMPDevice

NUM_CORES_CORTEX_A53 = 4


class DtslScript(DTSLv1):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=[
            ])
        ]
    
    def __init__(self, root):
        DTSLv1.__init__(self, root)
        
        '''Do not add directly to this list - first check if the item you are adding is already present'''
        self.mgdPlatformDevs = []
        
        # locate devices on the platform and create corresponding objects
        self.discoverDevices()
        
        # only DAP device is managed by default - others will be added when enabling trace, SMP etc
        if self.dap not in self.mgdPlatformDevs:
            self.mgdPlatformDevs.append(self.dap)
        
        self.exposeCores()
        
        self.setupSimpleSyncSMP()
        self.setManagedDeviceList(self.mgdPlatformDevs)
    
    # +----------------------------+
    # | Target dependent functions |
    # +----------------------------+
    
    def discoverDevices(self):
        '''find and create devices'''
        
        dapDev = self.findDevice("ARMCS-DP")
        self.dap = CSDAP(self, dapDev, "DAP")
        
        cortexA53coreDev = 0
        self.cortexA53cores = []
        
        streamID = 0
        
        for i in range(0, NUM_CORES_CORTEX_A53):
            # create core
            cortexA53coreDev = self.findDevice("Cortex-A53", cortexA53coreDev+1)
            dev = Device(self, cortexA53coreDev, "Cortex-A53_%d" % i)
            self.cortexA53cores.append(dev)
            
    def exposeCores(self):
        for core in self.cortexA53cores:
            self.addDeviceInterface(self.createDAPWrapper(core))
    
    def setupSimpleSyncSMP(self):
        '''Create SMP device using RDDI synchronization'''
        
        # create SMP device and expose from configuration
        # Cortex-A53x4 SMP
        smp = RDDISyncSMPDevice(self, "Cortex-A53 SMP", self.cortexA53cores)
        self.addDeviceInterface(self.createDAPWrapper(smp))
    
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
        
    def updateDynamicOptions(self):
        '''Update the dynamic options'''
        
    # +------------------------------+
    # | Target independent functions |
    # +------------------------------+
    
    def createDAPWrapper(self, core):
        '''Add a wrapper around a core to allow access to AHB and APB via the DAP'''
        return MemoryRouter(
            [DapMemoryAccessor("AHB", self.dap, 0, "AHB bus accessed via AP_0 on DAP_0"),
             DapMemoryAccessor("APB", self.dap, 1, "APB bus accessed via AP_1 on DAP_0")],
            core)
    
class DtslScript_RVI(DtslScript):
    @staticmethod
    def getOptionList():
        return [
            DTSLv1.tabSet("options", "Options", childOptions=[
            ])
        ]


