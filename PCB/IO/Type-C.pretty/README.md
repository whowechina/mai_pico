# Type-C.pretty
KiCad library for various Type-C USB connectors

Currently included connectors:

[Wurth 632723300011](https://www.digikey.com/product-detail/en/wurth-electronics-inc/632723300011/732-9618-1-ND/5806673)

- 1.6mm PCB compatibility
- Through-hole secondary pins; theoretically fully hand-solderable
- Most expensive of the three

[HRO TYPE-C-31-M-12](https://lcsc.com/product-detail/USB-Type-C_TYPE-C-31-M-12-Female-16P-SMD_C165948.html)

- Extremely cheap
- Simplified design with only one row; hand-solderable
- Legs do not extend long enough to "officially" support 1.6mm PCBs (Can be soldered in, but legs will be shorter than PCB)

[Amphenol 12401598E4#2A](https://www.digikey.com/product-detail/en/amphenol-commercial-products/12401598E4-2A/12401598E4-2ACT-ND/6051824)

- 1.6mm PCB compatibility
- Half the cost of the Wurth connector
- Requires reflow station or similar to solder full set of SMT pins below barrel

[GCT USB4085-GF-A](https://www.digikey.com/product-detail/en/gct/USB4085-GF-A/2073-USB4085-GF-ACT-ND/9859733)

- Only pins for USB 2.0
- All pins are through hole and are hand solderable
- Data pins barely break the surface of 1.6mm PCB
- Half the cost of the Amphenol
