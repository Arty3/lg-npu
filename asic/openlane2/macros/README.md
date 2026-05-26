# sky130 OpenRAM macros

This directory is the staging area OpenLane2 reads from (see the
`EXTRA_LEFS` / `EXTRA_GDS_FILES` / `EXTRA_LIBS` / `EXTRA_VERILOG_MODELS`
entries in `asic/openlane2/config.json`).

Do not check artefacts into git - the macros are large and have their own
upstream repo. Populate this directory with:

```bash
make asic-sky130-macros
```

That target invokes `asic/scripts/stage_macros.sh`, which clones
`https://github.com/efabless/sky130_sram_macros` at the ref pinned in
`asic/scripts/versions.env` and copies the following files for every macro
listed there:

```
<macro>.lef
<macro>.gds
<macro>.v
<macro>_<corner>.lib
```

Current set: `sky130_sram_4kbyte_1rw1r_32x1024_8`,
`sky130_sram_8kbyte_1rw1r_32x2048_8`, typical corner `TT_1p8V_25C`.

If you need a different corner or an additional macro, edit
`asic/scripts/versions.env` and rerun the target.
