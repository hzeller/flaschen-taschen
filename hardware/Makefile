
pi-mounting-rig.dxf:

%.dxf: %.ps
	pstoedit -psarg "-r600x600" -dt -f "dxf_s:-mm -ctl" $< $@

%.dxf: %.pdf
	pstoedit -psarg "-r600x600" -dt -f "dxf_s:-mm -ctl" $< $@
