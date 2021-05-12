all: \
	ACP342xC.bin \
	ACP342xD.bin \
	ACP344xV2.bin \
	AXM35xx_800_RDIMM.bin \
	AXM35xx_800_UDIMM.bin \
	AXM35xx_933_RDIMM.bin \
	AXM35xx_933_UDIMM.bin \
	AXM55xx_emu.bin \
	AXM55xx_1p0.bin \
	AXM55xx_1p1.bin \
	AXM55xx_srio.bin \
	AXM5600_emu.bin \
	AXM5600.bin \
	AXM5600_retention.bin \
	AXM5600_rc_1.bin \
	AXM5600_rc_2.bin \
	AXM5600_rc_3.bin \
	AXM5600_rc_4.bin \
	AXM5600_rc_5.bin \
	AXM5600_rio_host.bin \
	AXM5600_rio_agent.bin \
	AXC6700_emu.bin \
	AXC6700.bin \
	AXC6700_retention.bin \
	AXC6700_rc_1.bin \
	AXC6700_rc_2.bin \
	AXC6700_refcpu.bin \
	AXC6700_slow.bin

ACP342xC.bin: ACP342xC.parameters
	ncpBootMem -v 4 -c 34xx -a image -r parameters $< $@

ACP342xD.bin: ACP342xD.parameters
	ncpBootMem -v 4 -c 34xx -a image -r parameters $< $@

ACP344xV2.bin: ACP344xV2.parameters
	ncpBootMem -v 4 -c 34xx -a image -r parameters $< $@

AXM35xx_800_RDIMM.bin: AXM35xx_800_RDIMM.parameters
	ncpBootMem -v 9 -c 35xx -a image -r parameters $< $@

AXM35xx_800_UDIMM.bin: AXM35xx_800_UDIMM.parameters
	ncpBootMem -v 9 -c 35xx -a image -r parameters $< $@

AXM35xx_933_RDIMM.bin: AXM35xx_933_RDIMM.parameters
	ncpBootMem -v 9 -c 35xx -a image -r parameters $< $@

AXM35xx_933_UDIMM.bin: AXM35xx_933_UDIMM.parameters
	ncpBootMem -v 9 -c 35xx -a image -r parameters $< $@

AXM55xx_emu.bin: AXM55xx_emu.parameters
	ncpBootMem -v 7 -c 55xx -a image -r parameters $< $@

AXM55xx_1p0.bin: AXM55xx_1p0.parameters
	ncpBootMem -v 9 -c 55xx -a image -r parameters $< $@

AXM55xx_1p1.bin: AXM55xx_1p1.parameters
	ncpBootMem -v 9 -c 55xx -a image -r parameters $< $@

AXM55xx_srio.bin: AXM55xx_srio.parameters
	ncpBootMem -v 9 -c 55xx -a image -r parameters $< $@

AXM5600_emu.bin: AXM5600_emu.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600.bin: AXM5600.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_retention.bin: AXM5600_retention.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rc_1.bin: AXM5600_rc_1.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rc_2.bin: AXM5600_rc_2.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rc_3.bin: AXM5600_rc_3.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rc_4.bin: AXM5600_rc_4.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rio_host.bin: AXM5600_rio_host.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rio_agent.bin: AXM5600_rio_agent.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXM5600_rc_5.bin: AXM5600_rc_5.parameters
	ncpBootMem -v 9 -c 56xx -a image -r parameters $< $@

AXC6700_emu.bin: AXC6700_emu.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700.bin: AXC6700.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700_retention.bin: AXC6700_retention.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700_refcpu.bin: AXC6700_refcpu.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700_slow.bin: AXC6700_slow.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700_rc_1.bin: AXC6700_rc_1.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@

AXC6700_rc_2.bin: AXC6700_rc_2.parameters
	ncpBootMem -v 9 -c xlf -a image -r parameters $< $@
