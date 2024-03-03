# Mai Pico - Maimai 风格的迷你控制器
[Click here for the English version of this guide.](README.md)

<img src="doc/main.jpg" width="80%">  
<img src="doc/assembled.jpg" width="80%">

为了省事，本文档是我从原先英文版翻译回中文的，高度依赖了 GitHub Copilot (GPT)，所以语气可能怪怪的，见谅。

**特性:**
* 使用 15.6 英寸的便携式屏幕制作。
* 类似街机的体验。
* 可以双人游戏。
* 所有源文件开放。

感谢许多尊敬的爱好者和公司将他们的工具或材料免费或开源（KiCad，OnShape，InkScape，Raspberry 相关工具, 嘉立创）。

## 注意事项
这个项目是我众多项目里最复杂的一个。
* 它需要定制蚀刻的 ITO 玻璃和便携式显示器。
* 严重依赖 3D 打印。
* 需要焊接微小组件的技能。
* 需要许多其他 DIY 动手技能。
* IO PCB 仍处于原型状态，它可以工作，但布局和连接器并不完美。您可能需要直接从 Pi Pico 的引脚焊接。我太懒了，原型之后就没有做新版本。
* 我没有设计外壳，需要你自己来设计，让它看起来漂亮且结构稳定，发挥你的想象力，跟随你的品味。

## 查看我的其他项目
你也可以查看我其他的酷炫项目。

<img src="https://github.com/whowechina/popn_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/iidx_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/iidx_teeny/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/chu_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/mai_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/diva_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/aic_pico/raw/main/doc/main.jpg" height="100px">

* Popn Pico: https://github.com/whowechina/popn_pico
* IIDX Pico: https://github.com/whowechina/iidx_pico
* IIDX Teeny: https://github.com/whowechina/iidx_teeny
* Chu Pico: https://github.com/whowechina/chu_pico
* Mai Pico: https://github.com/whowechina/mai_pico
* Diva Pico: https://github.com/whowechina/diva_pico
* AIC Pico: https://github.com/whowechina/aic_pico

## **声明** ##
我在个人时间内制作了这个项目，没有任何经济利益或赞助。我将继续改进这个项目。我已尽我所能确保所有内容的准确性和功能性，但总有可能出现错误。如果你因使用这个开源项目而造成时间或金钱的损失，我不能负责。感谢你的理解。

查看我 GitHub 主页，上面有很多其他项目。

https://github.com/whowechina/

## 关于许可证
它是 CC-NC 授权。所以你只能给自己和你的朋友 DIY，不能利用这个项目赚钱。

## 如何构建
### PCB
* 去 JLCPCB 并使用 gerber zip 文件（最新的 `Production\PCB\mai_io_v*.zip` 和 `Production\PCB\mai_button_v*.zip`）下单，常规 FR-4 板，厚度 1.6mm。你需要 1x io PCB 和 8x 按钮 PCBs 来构建一个 Mai Pico con。  
  <img src="doc/pcbs.jpg" width="50%">
* 1x Raspberry Pico Pi Pico 或 Pico W。  
  https://www.raspberrypi.com/products/raspberry-pi-pico
  
  注意另一侧的 3 个引脚，它们很难焊接，可能会留下气泡。  
  <img src="doc/solder_usb_txrx.jpg" width="60%">
* 1x USB Type-C 插座（918-418K2023S40001 或 KH-TYPE-C-16P）
* 2x 0603 5.1kohm 电阻（R20，R21）用于 USB，如果你打算直接使用 Pi Pico 上面原生的 USB 插座，那么 Type-C 插座和电阻都可以省去，焊接也会简单不少。

* 3x MPR121 模块，市场上有许多类型，选择像这样的。
  https://www.sparkfun.com/products/retired/9695 

  在将 MP121 模块焊接到主 PCB 板之前，记得用小刀**切断 ADDR 和 GND 之间的微小连线**，注意绝对不要切过头。  
  <img src="doc/mpr121_cut.png" width="40%">
* 因为我们使用的是 MPR121 模块，而不是芯片，所以 U5、U6、U7 区域的离散组件留空。
* 1x SN74LV1T34DBVR（SOT-23-5）电平转换器（U8），如果你找不到，可以用 0603 10ohm 电阻（R4）替代。
  https://www.lcsc.com/product-detail/Buffer-Driver-Transceiver_Texas-Instruments-SN74LV1T34DBVR_C100024.html
* 4x 0603 1uF（0.1~1uF 都可以）电容器（C1，C2，C3，C7），可选，推荐。
* 16x WS2812B-3528 RGB LED（每个按钮需要两个）。
* 8x Kailh Choc v1 键盘开关，线性的，30gf 到 45gf 的。
  https://www.kailhswitch.com/mechanical-keyboard-switches/low-profile-key-switches/burnt-orange-switch.html

### ITO 玻璃
* 找一个服务定制蚀刻 ITO 镀膜玻璃。AutoCAD 文件是 `Production\CAD\mai_pico_ito_v*.dwg`。使用 2mm 厚，10-20ohm 平方电阻的 ITO 镀膜玻璃。  
  <img src="doc/ito_glass.jpg" width="70%">
* ITO 镀膜玻璃通过 "斑马线"（1.6mm 间距：0.8mm 黑色部分和 0.8mm 透明部分）连接到 IO PCB。使用 200°C 的硅加热头将斑马线粘贴到 PCB 的金手指部分和 ITO 镀膜玻璃上。这种线的正式名称是 "热压密封连接器"。  
  <img src="doc/zebra_cable.jpg" width="70%">
* 定制 ITO 镀膜玻璃相对较贵，但我们的尺寸小，所以并不像街机那么贵。这是我订购 ITO 玻璃的店铺，我订购的时候的最小批量是 5 件。据我所知，他们只在中国提供服务。我和他们店铺没有关联，此处提供链接并不表示我为他们的产品和服务负责。  
  https://shop378788148.taobao.com/?spm=2013.1.1000126.2.305e16c4LFf1GW

### 按键环
* 从 `Production\3DPrint\mai_*.stl` 打印出 8 套 `底座`、`连接板`、`按钮` 和 `盖子`。
* 购买 8 根 2mm（直径）* 40mm（长度）的钢轴，它们用作按钮铰链。
* 这是如何组装它们的方法，这个渲染图像中没有显示铰链轴和 PCB 上的组件。  
  <img src="doc/assemble.jpg" width="80%">
* 这是我如何组装环的方法。请注意，这些图片中缺少按钮开关的线路。
  * 所有离散组件已准备好。  
    <img src="doc/assemble_1.jpg" width="60%">
  * 首先焊接按钮 PCB。  
    <img src="doc/assemble_2.jpg" width="60%">
  * PCB 使用短且柔软的 4 线电缆串联，它们是 LED_GND、LED、LED_5V 和 BUTTON_GND。BUTTON 信号引脚还没有焊接，你可以后面再焊接。请不用担心按钮 GPIOs 的顺序，因为它们可以通过命令行界面重新映射。    
    <img src="doc/assemble_3.jpg" width="60%">
  * 你需要 3M5423 UHMW 胶带（或类似的硬且超滑的 PTFE 胶带，厚度 0.2-0.3）。它用来润滑触摸键开关的按钮表面。  
    <img src="doc/button_lub.jpg" width="60%">
  * 将 3D 打印的部件和 PCBs 组装在一起。  
    <img src="doc/assemble_4.jpg" width="60%">
  * 使用轴稍微扩大支撑孔，然后涂上一些键盘开关润滑剂，如 Krytox 205G0，使轴超级光滑。  
    <img src="doc/assemble_5.jpg" width="60%">
  * 每个 `连接板` 需要 8x M2*4mm 螺丝将两个 `底座` 连接在一起。  
    <img src="doc/assemble_6.jpg" width="60%">
  * 安装 `盖子`，每个需要 2x M2*5mm 螺丝。  
    <img src="doc/assemble_7.jpg" width="60%">

### 便携显示器
  * 分辨率：1920*1080，尺寸：15.6 英寸，刷新率：120Hz（推荐）或 60Hz，内置扬声器。
  * 它们很容易找到。选择闲鱼上有小瑕疵的会便宜得多，也一样很好用。
  * 有两种类型的驱动板，一种只有 micro-HDMI，另一种支持 type-C 显示（方便连接 Nintendo Switch）。两种都可以。  
  https://www.amazon.com/s?k=portable+display+15.6+120hz&s=price-asc-rank&crid=1CCA2EAYLZDBE&qid=1697276847&sprefix=portable+display+15.6+120hz%2Caps%2C387&ref=sr_st_price-asc-rank&ds=v1%3AqoS0mks05q225yuL8reh50fEEBkTZ583nMPEZJwjXAk

### 粘合到一起
  * 我使用 3M VHB 胶带（0.5mm 或 1mm 厚）将按钮环、ITO 玻璃和显示器粘在一起。
  * 只使用一点胶带，我使用 4 小块胶带将 ITO 玻璃粘到显示器上，另外 4 小块胶带将按钮环粘到 ITO 玻璃上。
  * 不要在 ITO 玻璃的走线上贴胶带，你可以在某个角度光线下看到 ITO 的走线。

### 外壳
  * 设计并使其看起来漂亮是你的工作。

### 固件
* UF2 文件在 `Production\Firmware` 文件夹中。
* 对于新的构建，连接 USB 到 PC 时按住 BOOTSEL 按钮，会出现一个名为 "RPI-RP2" 的磁盘。将 UF2 固件二进制文件拖入其中。就这样。
* LED 和 Touch 协议的实现遵循 Sucareto 在 https://github.com/Sucareto/Mai2Touch 上的研究。
* 它有一个命令行进行配置。你可以使用这个 Web Serial Terminal 连接到 Mai Pico 的 USB 串口。（注意："?" 是帮助）  
  https://googlechromelabs.github.io/serial-terminal/  
  <img src="doc/cmd.png" width="80%">
* 可以使用 `gpio` 命令重新映射按钮 GPIOs，固件支持 8 个主按钮和 4 个辅助按钮（分别是：Test, Service, Navigate, Coin）。
* 可以使用 `rgb` 命令为每个按钮分配串联的 RGB LED 数量。
* 可以通过 `level` 命令调整 LED 的亮度。
* 有 MPR121 参数调整和灵敏度设置，自己探索。
* 它实现了 3 个 COM 端口，一个用于命令行，另外两个用于 LED 和 Touch。通过向命令行发出 `whoami`，每个 COM 端口都会打印它们的身份。
* 按钮信号通过 HID Joystick 或 HID NKRO（键盘）发送到主机。有两套 NKRO 键映射，使用 `hid <joy|key1|key2>` 在它们之间切换。在 NKRO 模式下，键映射是：
  * key1：`WEDCXZAQ`-按键环，`3`-Select
  * key2（小键盘）：`89632147`-按键环，`*`-Select
  * 上述两套都有：`F1`-Test `F2`-Service `F3`-投币

## CAD 源文件
我正在使用 OnShape 的免费订阅。它很强大，但不能将原始设计存档到本地，所以我只能在这里分享链接。STL/DXF/DWG 文件是从这个在线文档导出的。  
https://cad.onshape.com/documents/d8b39d27c9cb7990d9ce4d46/w/2c1baa71e391bfd1246f122b/e/f87f0f1c373fe2186ddc5c9c?renderMode=0&uiState=652a665608b4e07137e3861a
