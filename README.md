### About
This is a tool to download (and convert) picture data from Polaroid Digital 320
cameras. The data on the camera is stored in a proprietary JPEG-like format.
Not enough information is known about the format to get the same level of
quality as the Windows drivers output. The color output will be a bit pale,
since the quantization factors (or tables?) are not known.

### Installation
Note that the tool is only tested on Linux and may require some tweaking to
work on other Unix-like platforms.

A Makefile is included to provide easy compilation on systems that use the GNU
tools. Just type "make" in the directory and copy the "polaroid" binary to a
suitable location.

### Picture Format Notes
The picture data format seems to be generic JPEG data without headers, but some
rules in the JPEG standard are violated as described below.

The blocks decoded from the huffman stream are arranged Y, Cb, Cr, Y and so
forth. Block 1 and 4 are luminance and block 3 and 4 are chrominance, but in a
standard JPEG picture the different components seems to follow each other.

The DC coefficient difference is tracked for every fourth block, meaning that
the luminance in block 1 is not tracked together with the luminance in block 4.
In a standard JPEG picture the difference is tracked for each component.

Only (AC and DC) luminance huffman tables are used, even for chrominance.

When comparing luminance output against pictures produced by the Windows TWAIN
drivers, the evidence suggests that some post processing like despeckle and
sharpen is supposed to be applied to the picture for best quality.

