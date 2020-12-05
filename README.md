# Nordic Semiconductor usbd cdc acm scanner logging

In the interest of expanding availability of examples for this device, I have 
created this usbd_scanner example.  This scans for advertisements and publishes them to a USB CDC ACM port.

This works for the nRF52840 DK (PCA10056) and nRF5280 Dongle (PCA10059).

This example is based on the one found in nRF5 SDK version 17.0.2
Please see the Nordic Semiconductor InfoCenter for more information.

I have only tested the Segger Embedded Studio build process (.emProject).
For my tests I used version 5.30

Please note: I have edited the .emProject file to reference an absolution location for the nRF5 SDK files.
I find this more convenient than building 'in' the Nordic example tree.
To use this:
1. Open Segger Embedded Studio
2. Click on 'Tools' on the menubar
3. Click on the 'Building' section.
4. Click on the ellipsis (...) in the 'Global Macros' option.
5. Add the following entry

  nRF5_SDK=PATH_TO_SDK

  e.g.

  nRF5_SDK=/nRF5_17.0.2

  in my case, since I have the SDK in my root directory

and click the Ok button save.

Rather than creating a global macro, one can define a macro per project.

