## CH32V_Color_LCD ##

<b>What is it?</b><br>
A set of functions to drive Sitronix SPI LCDs from CH32V RISC-V MCUs using the MounRiver Studio IDE development environment.<br>
<br>
<b>Why did you write it?</b><br>
I'm creating a bunch of projects on the CH32V line of MCUs and most have displays. My latest project uses a color LCD and needs an efficient set of code to drive it.<br>
<br>
<b>What's special about it?</b><br>
This code will explore using all of the resources of the CH32V to get maximum performance. The current code implements a double-buffer aka "ping-pong" DMA arrangement to get maximum speed. The DMA controller can be writing pixel data to the display while the MCU is simultaneously preparing the next set of pixel data. This allows for continous updates without delays.<br>
<br>
<b>Current project status?</b><br>
The code right now targets the ST7735S (160x80 Color LCD) on the CH32V203 w/20K SRAM. I'm waiting on PCBs to be received to use the ST7735 in one of my CO2 sensing projects. The photo below is of a prototype rig running the latest code on a CH32V203 driving a ST7735S 160x80 LCD.<br>

<b>Where does it go from here?</b><br>
I'm going to continue to add features as needed for my projects and encourage feedback for feature requests and code submissions to continuously improve it. It can easily support other Sitronix LCDs (e.g. ST7789) with minor changes.<br>
<br>
![CH32V_Color_LCD](/photo.jpg?raw=true "CH32V203+ST7735S")
<br>


