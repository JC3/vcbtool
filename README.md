# vcbtool
 misc vcb tools

## Blueprint <-> Image

To generate an image from a blueprint string:

1. Paste blueprint string.
2. Choose layer that you want to extract.
3. Click "Save As Image"

To generate a blueprint string from an image:

1. Choose layer that you want the image to be on.
2. Click "Create From Image"
3. Copy blueprint string.

## Generate ROM

To generate a ROM circuit from a binary data file:

1. Click "Load File" and choose file
2. Enter parameters
3. Click "Generate"
4. Copy blueprint string

Parameters:

- Word Size: The size of a single data word *in the data file*, in bytes.
- Endianness: The endianness of the data words *in the data file*.
- Address Bits: Number of address bits. This determines the number of words read from the file and the number of words stored in the ROM. Large values will make the ROM huge.
- Data Bits: Number of data bits to put in ROM. The least significant bits of the data words will be used.

Notes:

- If data file is larger than address space, only the first however many words will be read from the data file.
- If data file is smaller than address space, it will be padded with zeroes. Support for truncating the ROM circuit itself to smaller datasets is a TODO.
- The LSB of the address will be at the bottom of the ROM circuit.
- The LSB of the data will be at the top of the ROM circuit. Support for reversing data bit order is a TODO.

## Analysis Tool

To generate a GraphViz graph from a blueprint:

1. Paste blueprint string
2. Set settings
3. Click "Generate graph"
4. Copy generated GraphViz file

Parameters:

- Clean Graph: If selected, traces that don't affect the behavior of the circuit (i.e. trace nodes with a single input and at least one output) will be removed from the graph.

To generate a static analysis problem report from a blueprint:

1. Paste blueprint string
2. Set settings
3. Click "Check problems"
4. Read report

Parameters:

- Unconnected Traces: List traces that aren't written to or aren't read from
- <2-Input Gates: Warn about gates that normally have 2 inputs (like AND) but only have 1

---

Thanks ErikBot on Discord for the [general ROM design](https://www.youtube.com/watch?v=0oq0s3bW5Zk).
