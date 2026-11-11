"""Generate/patch KiCad schematics for the ArcadePortenta carrier.

Reads official KiCad 10 symbol libs for geometry, emits:
  - portenta.kicad_sch (regenerated)
  - vga.kicad_sch      (regenerated: R-ladder DAC + sync buffers + DE15)
  - inputs.kicad_sch   (new: JAMMA edge + 74HC165 chain)
  - power.kicad_sch    (new: 5V/12V entry, protection, chassis tie)
  - ArcadePortenta.kicad_sch (root: add two sheet blocks)
  - audio.kicad_sch    (patched in place: global I2S labels, bead, net-tie,
                        jack, XSMT, strap pins)
Connectivity is done with labels and power symbols placed exactly on pin
connection points; only decorative short wires are drawn.
"""
import re, sys, uuid, os

LIBDIR = r"C:\Program Files\KiCad\10.0\share\kicad\symbols"
PROJ   = os.path.dirname(os.path.abspath(__file__))  # project folder = this script's folder
ROOT_UUID = "c9a3af45-d4ca-41f4-a93e-a48562308a1b"
SHEET_UUIDS = {
    "portenta": "b7b4769a-eb1b-43e3-9622-47e600f56269",
    "vga":      "967ead6e-339b-4b6c-979f-6c11884eb0ac",
    "audio":    "39869425-8542-4a77-8405-5328b9fc851e",
    "power":    str(uuid.uuid4()),
    "inputs":   str(uuid.uuid4()),
}
FILE_UUIDS = {
    "portenta": "c192d843-1e52-4424-99b7-d338044b8b3c",
    "vga":      None,   # read from existing file
    "power":    str(uuid.uuid4()),
    "inputs":   str(uuid.uuid4()),
}

def U(): return str(uuid.uuid4())

# ---------------------------------------------------------------- s-expr utils
def read_file(p):
    with open(p, encoding="utf-8") as f: return f.read()

def find_block(text, header):
    """Return the balanced s-expr block starting at first occurrence of header."""
    i = text.find(header)
    if i < 0: return None
    depth = 0; j = i
    while j < len(text):
        c = text[j]
        if c == '"':                      # skip strings
            j += 1
            while j < len(text) and text[j] != '"':
                j += 2 if text[j] == '\\' else 1
        elif c == '(': depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0: return text[i:j+1]
        j += 1
    return None

_libcache = {}
def harvest(lib, name):
    """Return (embedded_block, pins) for lib symbol. pins: list of
    (unit, number, x, y, ang)."""
    key = (lib, name)
    if key in _libcache: return _libcache[key]
    txt = read_file(os.path.join(LIBDIR, lib + ".kicad_sym"))
    blk = find_block(txt, '(symbol "%s"' % name)
    if blk is None:
        raise SystemExit("symbol %s:%s not found" % (lib, name))
    m = re.search(r'\(extends "([^"]+)"\)', blk)
    if m:
        parent = m.group(1)
        pblk, ppins = harvest(lib, parent)
        # graft: use parent geometry under child name
        blk = pblk.replace('"%s:%s"' % (lib, parent), '"%s:%s"' % (lib, name), 1)
        blk = blk.replace('"%s_' % parent, '"%s_' % name)
        _libcache[key] = (blk, ppins)
        return _libcache[key]
    blk = blk.replace('(symbol "%s"' % name, '(symbol "%s:%s"' % (lib, name), 1)
    pins = []
    for sm in re.finditer(r'\(symbol "%s_(\d+)_\d+"' % re.escape(name), blk):
        unit = int(sm.group(1))
        sub = find_block(blk[sm.start():], sm.group(0))
        for pm in re.finditer(
            r'\(pin \w+ \w+\s*\(at (-?[\d.]+) (-?[\d.]+) (\d+)\)[\s\S]*?\(number "([^"]+)"',
            sub):
            x, y, ang, num = float(pm.group(1)), float(pm.group(2)), int(pm.group(3)), pm.group(4)
            pins.append((unit, num, x, y, ang))
    _libcache[key] = (blk, pins)
    return _libcache[key]


def snap(v):
    return round(round(v / 1.27) * 1.27, 2)

# ---------------------------------------------------------------- sheet writer
class Sheet:
    def __init__(self, name, paper="A4"):
        self.name = name
        self.paper = paper
        self.defs = {}      # "Lib:Name" -> block
        self.items = []
        self.pwr_n = {"portenta":100, "vga":200, "inputs":300, "power":400}.get(name, 500)

    def _def(self, lib, sym):
        blk, pins = harvest(lib, sym)
        self.defs["%s:%s" % (lib, sym)] = blk
        return pins

    def place(self, lib, sym, ref, val, x, y, fp="", unit=1, dnp=False):
        """Place symbol unit at (x,y) rot 0. Returns {pinnum:(wx,wy,ang)} for
        pins of this unit (and unit 0 commons)."""
        x, y = snap(x), snap(y)
        pins = self._def(lib, sym)
        world = {}
        for (u, num, px, py, ang) in pins:
            if u in (unit, 0):
                world[num] = (round(x + px, 2), round(y - py, 2), ang)
        self.items.append(SYM_T.format(
            lib=lib, sym=sym, x=x, y=y, ref=ref, val=esc(val), fp=esc(fp),
            unit=unit, uuid=U(), dnp="yes" if dnp else "no",
            path="/%s/%s" % (ROOT_UUID, SHEET_UUIDS[self.name])))
        return world

    def pwr(self, net, x, y):
        x, y = snap(x), snap(y)
        lib = "power"
        pins = self._def(lib, net)
        px, py = pins[0][2], pins[0][3]
        ref = "#PWR%03d" % self.pwr_n; self.pwr_n += 1
        self.items.append(PWR_T.format(
            net=net, x=round(x - px, 2), y=round(y + py, 2), ref=ref, uuid=U(),
            path="/%s/%s" % (ROOT_UUID, SHEET_UUIDS[self.name])))

    def flag(self, x, y):
        self.pwr("PWR_FLAG", x, y)

    def glabel(self, text, x, y, ang=0, shape="input"):
        x, y = snap(x), snap(y)
        just = {0: "left", 180: "right", 90: "left", 270: "right"}[ang]
        self.items.append(GLBL_T.format(t=esc(text), x=x, y=y, a=ang,
                                        just=just, shape=shape, uuid=U()))

    def llabel(self, text, x, y, ang=0):
        x, y = snap(x), snap(y)
        just = {0: "left", 180: "right", 90: "left", 270: "right"}[ang]
        self.items.append(LLBL_T.format(t=esc(text), x=x, y=y, a=ang,
                                        just=just, uuid=U()))

    def nc(self, x, y):
        x, y = snap(x), snap(y)
        self.items.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n'
                          % (x, y, U()))

    def wire(self, x1, y1, x2, y2):
        self.items.append(
            '\t(wire\n\t\t(pts\n\t\t\t(xy %s %s) (xy %s %s)\n\t\t)\n'
            '\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n'
            '\t\t(uuid "%s")\n\t)\n' % (x1, y1, x2, y2, U()))

    def note(self, text, x, y, size=1.27):
        self.items.append(
            '\t(text "%s"\n\t\t(exclude_from_sim no)\n\t\t(at %s %s 0)\n'
            '\t\t(effects\n\t\t\t(font\n\t\t\t\t(size %s %s)\n\t\t\t)\n'
            '\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "%s")\n\t)\n'
            % (esc_multiline(text), x, y, size, size, U()))

    def emit(self, file_uuid):
        out = ['(kicad_sch\n\t(version 20250114)\n\t(generator "eeschema")\n'
               '\t(generator_version "9.0")\n\t(uuid "%s")\n\t(paper "%s")\n'
               % (file_uuid, self.paper)]
        out.append("\t(lib_symbols\n")
        for blk in self.defs.values():
            out.append(indent(blk, 2) + "\n")
        out.append("\t)\n")
        out.extend(self.items)
        out.append('\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n')
        out.append("\t(embedded_fonts no)\n)\n")
        return "".join(out)

def esc(s): return s.replace('"', '\\"')
def esc_multiline(s): return s.replace('"', '\\"').replace("\n", "\\n")
def indent(blk, n):
    pad = "\t" * n
    return "\n".join(pad + l for l in blk.splitlines())

SYM_T = '''\t(symbol
\t\t(lib_id "{lib}:{sym}")
\t\t(at {x} {y} 0)
\t\t(unit {unit})
\t\t(exclude_from_sim no)
\t\t(in_bom yes)
\t\t(on_board yes)
\t\t(dnp {dnp})
\t\t(uuid "{uuid}")
\t\t(property "Reference" "{ref}"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(justify left)
\t\t\t)
\t\t)
\t\t(property "Value" "{val}"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(justify left)
\t\t\t)
\t\t)
\t\t(property "Footprint" "{fp}"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(hide yes)
\t\t\t)
\t\t)
\t\t(property "Datasheet" "~"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(hide yes)
\t\t\t)
\t\t)
\t\t(instances
\t\t\t(project "ArcadePortenta"
\t\t\t\t(path "{path}"
\t\t\t\t\t(reference "{ref}")
\t\t\t\t\t(unit {unit})
\t\t\t\t)
\t\t\t)
\t\t)
\t)
'''

PWR_T = '''\t(symbol
\t\t(lib_id "power:{net}")
\t\t(at {x} {y} 0)
\t\t(unit 1)
\t\t(exclude_from_sim no)
\t\t(in_bom yes)
\t\t(on_board yes)
\t\t(dnp no)
\t\t(uuid "{uuid}")
\t\t(property "Reference" "{ref}"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(hide yes)
\t\t\t)
\t\t)
\t\t(property "Value" "{net}"
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t)
\t\t)
\t\t(property "Footprint" ""
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(hide yes)
\t\t\t)
\t\t)
\t\t(property "Datasheet" ""
\t\t\t(at {x} {y} 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(hide yes)
\t\t\t)
\t\t)
\t\t(instances
\t\t\t(project "ArcadePortenta"
\t\t\t\t(path "{path}"
\t\t\t\t\t(reference "{ref}")
\t\t\t\t\t(unit 1)
\t\t\t\t)
\t\t\t)
\t\t)
\t)
'''

GLBL_T = '''\t(global_label "{t}"
\t\t(shape {shape})
\t\t(at {x} {y} {a})
\t\t(effects
\t\t\t(font
\t\t\t\t(size 1.27 1.27)
\t\t\t)
\t\t\t(justify {just})
\t\t)
\t\t(uuid "{uuid}")
\t)
'''

LLBL_T = '''\t(label "{t}"
\t\t(at {x} {y} {a})
\t\t(effects
\t\t\t(font
\t\t\t\t(size 1.27 1.27)
\t\t\t)
\t\t\t(justify {just} bottom)
\t\t)
\t\t(uuid "{uuid}")
\t)
'''

# =====================================================================
# Pin signal tables  (signal, kind) kind: glabel/pwr/nc
# Derived from Arduino pinout PDF geometry + breakout lib + CubeMX DB.
GL = "gl"; PW = "pw"; NC = "nc"
J1 = {
 1:(NC,"ETH_A+"), 3:(NC,"ETH_A-"), 5:(NC,"ETH_B+"), 7:(NC,"ETH_B-"),
 9:(NC,""), 11:(NC,""), 13:(NC,""), 15:(NC,""), 17:(NC,"ETH_L1"),
 19:(NC,"ETH_L2"), 21:(PW,"+5V"), 23:(NC,"USB1_VBUS"), 25:(NC,"USB1_D+"),
 27:(NC,"USB1_D-"), 29:(GL,"LTDC_R7"), 31:(PW,"GND"), 33:(GL,"DBG_TX"),
 35:(GL,"DBG_RX"), 37:(NC,"PI14"), 39:(NC,"PI15"), 41:(PW,"+5V"),
 43:(NC,"PB7-PMIC"), 45:(NC,"PB6-PMIC"), 47:(PW,"GND"), 49:(NC,"PH13"),
 51:(NC,"PB8"), 53:(NC,"VSYS"), 55:(GL,"SD_CLK"), 57:(GL,"SD_CMD"),
 59:(GL,"SD_D0"), 61:(GL,"SD_D1"), 63:(GL,"SD_D2"), 65:(GL,"SD_D3"),
 67:(NC,""), 69:(NC,""), 71:(NC,""), 73:(GL,"NRST"), 75:(GL,"SWDIO"),
 77:(GL,"SWCLK"), 79:(NC,"SWO=PB3"),
 2:(NC,""), 4:(NC,""), 6:(NC,""), 8:(NC,""), 10:(NC,"DSI"), 12:(NC,"DSI"),
 14:(NC,"DSI"), 16:(NC,"DSI"), 18:(NC,"DSI"), 20:(NC,"DSI"), 22:(PW,"GND"),
 24:(NC,""), 26:(NC,"USB0_D+"), 28:(NC,"USB0_D-"), 30:(NC,""),
 32:(PW,"+5V"), 34:(GL,"IN_LATCH"), 36:(NC,"PI9"), 38:(NC,"PI10"),
 40:(NC,"PI13"), 42:(PW,"GND"), 44:(NC,"PH8"), 46:(NC,"PH7"),
 48:(PW,"+5V"), 50:(NC,""), 52:(NC,""), 54:(PW,"GND"), 56:(GL,"I2S_CK"),
 58:(GL,"I2S_WS"), 60:(GL,"LTDC_G7"), 62:(GL,"I2S_SDO"), 64:(NC,"VSYS"),
 66:(NC,""), 68:(NC,""), 70:(NC,"PE2"), 72:(NC,"PB2"), 74:(GL,"VSD"),
 76:(NC,""), 78:(NC,""), 80:(NC,""),
}
J2 = {
 1:(NC,""), 3:(NC,""), 5:(NC,""), 7:(NC,"COINCELL"), 9:(NC,""), 11:(NC,""),
 13:(NC,""), 15:(NC,""), 17:(NC,""), 19:(NC,""), 21:(NC,""),
 23:(PW,"+3.3V"), 25:(NC,"PJ8"), 27:(GL,"LTDC_G2"), 29:(NC,""), 31:(NC,""),
 33:(PW,"GND"), 35:(NC,""), 37:(NC,""), 39:(NC,""), 41:(NC,""),
 43:(PW,"+3.3V"), 45:(NC,"PH12*"), 47:(NC,"PH11*"), 49:(NC,"PI5*"),
 51:(NC,"PI7*"), 53:(NC,"PI6*"), 55:(NC,""), 57:(PW,"GND"),
 59:(GL,"LTDC_B3"), 61:(GL,"HSYNC"), 63:(NC,"PC7"), 65:(GL,"PCLK"),
 67:(GL,"LTDC_G4"), 69:(PW,"+3.3V"), 71:(NC,"VREF+"), 73:(NC,"PA0*"),
 75:(NC,"PA1"), 77:(NC,"A2"), 79:(NC,"A3"),
 2:(GL,"LTDC_B7"), 4:(GL,"LTDC_B6"), 6:(GL,"LTDC_B4"), 8:(NC,"PH14"),
 10:(GL,"LTDC_R6"), 12:(GL,"LTDC_R5"), 14:(GL,"LTDC_R4"), 16:(GL,"LTDC_R3"),
 18:(GL,"LTDC_B5"), 20:(NC,"PA6"), 22:(GL,"VSYNC"), 24:(PW,"GND"),
 26:(NC,"PG14"), 28:(NC,"PG9"), 30:(NC,""), 32:(NC,""), 34:(PW,"+3.3V"),
 36:(GL,"LTDC_G5"), 38:(NC,"PI1"), 40:(GL,"IN_CLK"), 42:(GL,"IN_DATA"),
 44:(PW,"GND"), 46:(GL,"SD_CD"), 48:(GL,"LED_STAT"), 50:(GL,"XSMT"),
 52:(NC,"PD5-amp"), 54:(GL,"COIN_CNT"), 56:(GL,"MODE_JP"), 58:(NC,"PG10"),
 60:(GL,"LTDC_G6"), 62:(NC,"PH15"), 64:(NC,"PJ7"), 66:(GL,"LTDC_G3"),
 68:(NC,"PH6"), 70:(PW,"GND"), 72:(NC,"VREF-"), 74:(NC,"PC2*"),
 76:(NC,"PC3*"), 78:(NC,"PA4*"), 80:(NC,"PA6*"),
}
DF40_FP = "Connector_Hirose_DF40:Hirose_DF40HC(3.5)-80DS-0.4V_2x40_P0.4mm"

def do_connector(sh, world, table):
    for num, (kind, sig) in table.items():
        p = world.get(str(num))
        if p is None: raise SystemExit("pin %s missing" % num)
        x, y, ang = p
        lab_ang = 180 if ang == 0 else 0
        if kind == GL:
            sh.glabel(sig, x, y, lab_ang)
        elif kind == PW:
            sh.pwr(sig, x, y)
        else:
            sh.nc(x, y)

# =====================================================================
def gen_portenta():
    sh = Sheet("portenta", paper="A3")
    sh.note("Arduino Portenta H7 high-density connectors.\\n"
            "Pin map verified against ABX00042 full-pinout PDF (13/11/2024), "
            "Arduino_PortentaBreakout lib and STM32H747XIH6 CubeMX DB.\\n"
            "Pads marked Px* share an MCU ball with another pad (dual bond) - "
            "twin left NC:\\n"
            "  PB3 = J1-63 (SD_D2) = J1-79 (SWO): SWO unavailable.\\n"
            "  PA0 = J1-34 (IN_LATCH) = J2-73 (A0).  PA4 = J2-22 (VSYNC) = J2-78.\\n"
            "  PC2 = J2-40 = J2-74.  PC3 = J2-42 = J2-76.  PH11 = J2-12 = J2-47.\\n"
            "  PH12 = J2-10 = J2-45.  PI5 = J2-18 = J2-49.  PI6 = J2-4 = J2-53.  "
            "PI7 = J2-2 = J2-51.\\n"
            "J1-43/45 (I2C1) shared with on-module PMIC - leave NC.", 20, 275, 1.6)
    w1 = sh.place("Connector_Generic", "Conn_02x40_Odd_Even", "J1",
                  "PORTENTA_J1", 60, 130, DF40_FP)
    w2 = sh.place("Connector_Generic", "Conn_02x40_Odd_Even", "J2",
                  "PORTENTA_J2", 200, 130, DF40_FP)
    do_connector(sh, w1, J1)
    do_connector(sh, w2, J2)

    # SWD header (ST-Link), J3 - keeps the original 6-pin idea
    w = sh.place("Connector", "Conn_01x06_Pin", "J3", "SWD",
                 285, 40, "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical")
    for num, sig in {"1": ("pw", "+3.3V"), "2": ("gl", "SWDIO"),
                     "3": ("gl", "SWCLK"), "4": ("gl", "NRST"),
                     "5": ("pw", "GND"), "6": (None, None)}.items():
        x, y, ang = w[num]
        if sig[0] == "pw": sh.pwr(sig[1], x, y)
        elif sig[0] == "gl": sh.glabel(sig[1], x, y, 0)
        else: sh.nc(x, y)
    sh.note("SWO not available (PB3 used as SD_D2)", 280, 52)

    # Debug UART header J4
    w = sh.place("Connector_Generic", "Conn_01x03", "J4", "UART-DEBUG",
                 285, 70, "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical")
    sh.glabel("DBG_TX", *w["1"][:2], 0)
    sh.glabel("DBG_RX", *w["2"][:2], 0)
    sh.pwr("GND", *w["3"][:2])

    # microSD socket J5 on SDMMC2
    w = sh.place("Connector", "Micro_SD_Card_Det_Hirose_DM3AT", "J5", "microSD",
                 330, 130, "Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5")
    sdmap = {"1": ("gl", "SD_D2"), "2": ("gl", "SD_D3"), "3": ("gl", "SD_CMD"),
             "4": ("gl", "VSD"),   "5": ("gl", "SD_CLK"), "6": ("pw", "GND"),
             "7": ("gl", "SD_D0"), "8": ("gl", "SD_D1"),
             "9": ("gl", "SD_CD"), "10": ("pw", "GND"),
             "SH": ("gl", "CHASSIS")}
    for num, (k, s) in sdmap.items():
        if num not in w: continue
        x, y, ang = w[num]
        la = 180 if ang == 0 else 0
        if k == "gl": sh.glabel(s, x, y, la)
        else: sh.pwr(s, x, y)
    # SD_CD pull-up + VSD decoupling
    w = sh.place("Device", "R", "R100", "10k", 330, 165,
                 "Resistor_SMD:R_0603_1608Metric")
    sh.glabel("SD_CD", *w["2"][:2], 90); sh.pwr("+3.3V", *w["1"][:2])
    w = sh.place("Device", "C", "C100", "10uF", 345, 165,
                 "Capacitor_SMD:C_0805_2012Metric")
    sh.glabel("VSD", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Device", "C", "C101", "100nF", 355, 165,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.glabel("VSD", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Jumper", "SolderJumper_2_Open", "JP100", "VSD-3V3", 345, 180,
                 "Jumper:SolderJumper-2_P1.3mm_Open_TrianglePad1.0x1.5mm", dnp=True)
    sh.glabel("VSD", *w["1"][:2], 180); sh.pwr("+3.3V", *w["2"][:2])
    sh.note("VSD: module V-SDCARD pad feeds SD socket VDD.\\n"
            "Close JP100 only if V-SDCARD turns out not to source 3V3 "
            "(verify against Portenta schematic before closing!)", 320, 195)
    sh.flag(365, 165); sh.glabel("VSD", 365, 165, 0)

    # status LED
    w = sh.place("Device", "R", "R101", "330", 285, 95,
                 "Resistor_SMD:R_0603_1608Metric")
    sh.glabel("LED_STAT", *w["1"][:2], 90)
    rbot = w["2"][:2]
    w = sh.place("Device", "LED", "D100", "LED", 285, 107,
                 "LED_SMD:LED_0603_1608Metric")
    # connect R bottom to LED top via labels
    sh.llabel("led_k", *rbot, 0); sh.llabel("led_k", *w["2"][:2], 0)
    sh.pwr("GND", *w["1"][:2])

    # video mode jumper (PG3): open = VGA 31k, closed = CRT 15k
    w = sh.place("Jumper", "SolderJumper_2_Open", "JP101", "MODE 31k/15k",
                 285, 125, "Jumper:SolderJumper-2_P1.3mm_Open_TrianglePad1.0x1.5mm")
    sh.glabel("MODE_JP", *w["1"][:2], 180)
    sh.pwr("GND", *w["2"][:2])
    sh.note("PG3 mode select: open = VGA 640x480@60 (31.5 kHz),\\n"
            "closed = CRT 384x224@59.6 (15.6 kHz). Firmware pull-up.", 275, 135)

    # 3.3V flag (module is the source)
    sh.flag(240, 240); sh.pwr("+3.3V", 240, 240)
    sh.note("+3V3 sourced by Portenta module (J2-23/34/43/69).\\n"
            "Budget < 500 mA total for DAC/logic.", 235, 248)
    return sh.emit(FILE_UUIDS["portenta"])

# =====================================================================
LADDER = {
    "R": (["LTDC_R7", "LTDC_R6", "LTDC_R5", "LTDC_R4", "LTDC_R3"],
          ["536", "1.07k", "2.15k", "4.32k", "8.66k"], "VID_R"),
    "G": (["LTDC_G7", "LTDC_G6", "LTDC_G5", "LTDC_G4", "LTDC_G3", "LTDC_G2"],
          ["549", "1.1k", "2.21k", "4.42k", "8.87k", "17.8k"], "VID_G"),
    "B": (["LTDC_B7", "LTDC_B6", "LTDC_B5", "LTDC_B4", "LTDC_B3"],
          ["536", "1.07k", "2.15k", "4.32k", "8.66k"], "VID_B"),
}

def gen_vga():
    sh = Sheet("vga", paper="A3")
    sh.note("Video DAC: binary-weighted ladders, 3.3V CMOS -> 0.7Vpp into "
            "double-terminated 75R.\\nR_thevenin = 75*(3.3/0.7-1) = 278.6R; "
            "MSB = 278.6*1.9375 (5b) / *1.96875 (6b).\\n"
            "1% E96. Green 6 bit, red/blue 5 bit (RGB565).", 20, 20, 1.6)
    rn = 200
    x0 = 40
    for ci, (ch, (bits, vals, net)) in enumerate(LADDER.items()):
        y0 = 45 + ci * 45
        for i, (b, v) in enumerate(zip(bits, vals)):
            x = x0 + i * 12.7
            w = sh.place("Device", "R", "R%d" % rn, v, x, y0,
                         "Resistor_SMD:R_0603_1608Metric"); rn += 1
            sh.glabel(b, *w["1"][:2], 90)
            sh.glabel(net, *w["2"][:2], 270)

    # EMI caps at connector
    for i, net in enumerate(["VID_R", "VID_G", "VID_B"]):
        w = sh.place("Device", "C_Small", "C%d" % (200 + i), "22pF",
                     150 + i * 12.7, 60, "Capacitor_SMD:C_0603_1608Metric")
        sh.glabel(net, *w["1"][:2], 90)
        sh.pwr("GND", *w["2"][:2])

    # HSYNC/VSYNC buffers - 74HCT244 (74LS244 symbol, HCT part fitted)
    w = sh.place("74xx", "74LS244", "U200", "74HCT244", 210, 90,
                 "Package_SO:SOIC-20W_7.5x12.8mm_P1.27mm")
    sh.pwr("GND", *w["1"][:2])                     # OEa
    sh.pwr("GND", *w["19"][:2])                    # OEb
    sh.glabel("HSYNC", *w["2"][:2], 180)
    sh.glabel("VSYNC", *w["4"][:2], 180)
    sh.glabel("HSYNC_5V", *w["18"][:2], 0)
    sh.glabel("VSYNC_5V", *w["16"][:2], 0)
    for pin in ("6", "8", "11", "13", "15", "17"): sh.pwr("GND", *w[pin][:2])
    for pin in ("14", "12", "9", "7", "5", "3"):   sh.nc(*w[pin][:2])
    sh.pwr("+5V", *w["20"][:2]); sh.pwr("GND", *w["10"][:2])
    w = sh.place("Device", "C", "C203", "100nF", 240, 130,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.pwr("+5V", *w["1"][:2]); sh.pwr("GND", *w["2"][:2])

    # CSYNC = HSYNC_5V xor VSYNC_5V - 74HC86 @5V (fed by 5V CMOS outputs)
    w = sh.place("74xx", "74LS86", "U201", "74HC86", 280, 60,
                 "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm", unit=1)
    sh.glabel("HSYNC_5V", *w["1"][:2], 180)
    sh.glabel("VSYNC_5V", *w["2"][:2], 180)
    sh.glabel("CSYNC", *w["3"][:2], 0)
    for u, pins in ((2, ("4", "5")), (3, ("9", "10")), (4, ("12", "13"))):
        w = sh.place("74xx", "74LS86", "U201", "74HC86", 280, 60 + u * 25, unit=u)
        for p in pins: sh.pwr("GND", *w[p][:2])
        out = {"2": "6", "3": "8", "4": "11"}[str(u)]
        sh.nc(*w[out][:2])
    w = sh.place("74xx", "74LS86", "U201", "74HC86", 280, 170, unit=5)
    sh.pwr("+5V", *w["14"][:2]); sh.pwr("GND", *w["7"][:2])
    w = sh.place("Device", "C", "C204", "100nF", 300, 170,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.pwr("+5V", *w["1"][:2]); sh.pwr("GND", *w["2"][:2])

    # DE-15 VGA socket
    w = sh.place("Connector", "DE15_Socket_HighDensity_MountingHoles", "J7",
                 "VGA", 360, 90,
                 "Connector_Dsub:DSUB-15-HD_Female_Horizontal_P2.29x1.98mm_EdgePinOffset8.35mm_Housed_MountingHolesOffset10.89mm")
    de15 = {"1": ("gl", "VID_R"), "2": ("gl", "VID_G"), "3": ("gl", "VID_B"),
            "13": ("gl", "HSYNC_5V"), "14": ("gl", "VSYNC_5V"),
            "5": ("pw", "GND"), "6": ("pw", "GND"), "7": ("pw", "GND"),
            "8": ("pw", "GND"), "10": ("pw", "GND"),
            "4": (None, None), "9": (None, None), "11": (None, None),
            "12": (None, None), "15": (None, None)}
    for num, (k, s) in de15.items():
        if num not in w: continue
        x, y, ang = w[num]
        la = 180 if ang == 0 else 0
        if k == "gl": sh.glabel(s, x, y, la)
        elif k == "pw": sh.pwr(s, x, y)
        else: sh.nc(x, y)
    if "SH" in w:
        x, y, ang = w["SH"]
        sh.glabel("CHASSIS", x, y, 270)
    sh.note("DDC (pins 12/15) unused. Pin 9 (+5V key) NC.", 345, 130)

    # JAMMA video takeoff jumpers
    for i, (a, b) in enumerate((("VID_R", "JAMMA_R"), ("VID_G", "JAMMA_G"),
                                ("VID_B", "JAMMA_B"), ("CSYNC", "JAMMA_SYNC"))):
        w = sh.place("Jumper", "SolderJumper_2_Bridged", "JP%d" % (200 + i),
                     "%s" % b, 150, 90 + i * 12.7,
                     "Jumper:SolderJumper-2_P1.3mm_Bridged_RoundedPad1.0x1.5mm")
        sh.glabel(a, *w["1"][:2], 180)
        sh.glabel(b, *w["2"][:2], 0)
    sh.note("Open JAMMA jumpers when only the DE15 is used\\n"
            "(second 75R load halves video level).", 140, 145)

    # test points
    for i, net in enumerate(["PCLK", "HSYNC", "VSYNC", "CSYNC"]):
        w = sh.place("Connector", "TestPoint", "TP%d" % (200 + i), net,
                     150 + i * 15, 165, "TestPoint:TestPoint_Pad_D1.5mm")
        sh.glabel(net, *w["1"][:2], 90)
    return sh.emit(FILE_UUIDS["vga"])

# =====================================================================
JAMMA = {   # jamma pin (component 1-28 odd=2n-1 / solder A..f even=2n)
 1:(PW,"GND"), 2:(PW,"GND"), 3:(PW,"GND"), 4:(PW,"GND"),
 5:(GL,"JAMMA_5V"), 6:(GL,"JAMMA_5V"), 7:(GL,"JAMMA_5V"), 8:(GL,"JAMMA_5V"),
 9:(NC,"-5V"), 10:(NC,"-5V"), 11:(GL,"JAMMA_12V"), 12:(GL,"JAMMA_12V"),
 13:(NC,"KEY"), 14:(NC,"KEY"), 15:(GL,"COIN_CNT_OUT"), 16:(NC,"CNT2"),
 17:(NC,"LOCKOUT1"), 18:(NC,"LOCKOUT2"), 19:(NC,"SPK+"), 20:(NC,"SPK-"),
 21:(NC,""), 22:(NC,""), 23:(GL,"JAMMA_R"), 24:(GL,"JAMMA_G"),
 25:(GL,"JAMMA_B"), 26:(GL,"JAMMA_SYNC"), 27:(PW,"GND"), 28:(GL,"IN_SERVICE"),
 29:(GL,"IN_TEST"), 30:(GL,"IN_TILT"), 31:(GL,"IN_COIN1"), 32:(GL,"IN_COIN2"),
 33:(GL,"IN_START1"), 34:(GL,"IN_START2"), 35:(GL,"IN_P1U"), 36:(GL,"IN_P2U"),
 37:(GL,"IN_P1D"), 38:(GL,"IN_P2D"), 39:(GL,"IN_P1L"), 40:(GL,"IN_P2L"),
 41:(GL,"IN_P1R"), 42:(GL,"IN_P2R"), 43:(GL,"IN_P1B1"), 44:(GL,"IN_P2B1"),
 45:(GL,"IN_P1B2"), 46:(GL,"IN_P2B2"), 47:(GL,"IN_P1B3"), 48:(GL,"IN_P2B3"),
 49:(GL,"IN_P1B4"), 50:(GL,"IN_P2B4"), 51:(NC,""), 52:(NC,""),
 53:(PW,"GND"), 54:(PW,"GND"), 55:(PW,"GND"), 56:(PW,"GND"),
}
# 74HC165 loading order, chip 0 first (bit0 = first shifted in = QH of last?)
CHAIN = [
 ["IN_P1U", "IN_P1D", "IN_P1L", "IN_P1R", "IN_P1B1", "IN_P1B2", "IN_P1B3", "IN_P1B4"],
 ["IN_P1B5", "IN_P1B6", "IN_P2U", "IN_P2D", "IN_P2L", "IN_P2R", "IN_P2B1", "IN_P2B2"],
 ["IN_P2B3", "IN_P2B4", "IN_P2B5", "IN_P2B6", "IN_START1", "IN_START2", "IN_COIN1", "IN_COIN2"],
 ["IN_TEST", "IN_SERVICE", "IN_TILT", "IN_AUX1", "IN_AUX2", "IN_AUX3", "IN_AUX4", "IN_AUX5"],
]

def gen_inputs():
    sh = Sheet("inputs", paper="A2")
    sh.note("JAMMA edge: odd pins = component side 1-28, even = solder side "
            "A-f.\\n3.96 mm (0.156 inch) pitch card-edge - custom footprint, part "
            "of board outline.\\nButtons switch to GND; 10k pull-ups to 3V3; "
            "1k + 100n RC per input (fc=1.6kHz, tau=1ms debounce helper).\\n"
            "P1/P2 buttons 4-6 on CPS-style kick harness J9/J10; B4 also on "
            "JAMMA 25/c.", 20, 20, 1.8)
    w = sh.place("Connector_Generic", "Conn_02x28_Odd_Even", "J8",
                 "JAMMA-56", 60, 120, "")
    do_connector(sh, w, JAMMA)
    # coin counter driver (DNP)
    w = sh.place("Transistor_BJT", "MMBT2222A", "Q300", "MMBT2222A", 110, 220,
                 "Package_TO_SOT_SMD:SOT-23", dnp=True)
    sh.glabel("COIN_CNT_OUT", *w["3"][:2], 90)    # collector (top)
    sh.pwr("GND", *w["2"][:2])                    # emitter (bottom)
    base = w["1"][:2]
    w = sh.place("Device", "R", "R300", "1k", 95, 220,
                 "Resistor_SMD:R_0603_1608Metric", dnp=True)
    sh.llabel("cnt_b", *w["2"][:2], 0); sh.llabel("cnt_b", *base, 0)
    sh.glabel("COIN_CNT", *w["1"][:2], 90)
    w = sh.place("Device", "D", "D300", "1N4148W", 130, 220,
                 "Diode_SMD:D_SOD-123", dnp=True)
    sh.glabel("JAMMA_12V", *w["1"][:2], 180)
    sh.glabel("COIN_CNT_OUT", *w["2"][:2], 0)
    sh.note("Coin counter (DNP): PE3 -> Q300 low-side, flyback to +12V "
            "(counter coil supply).", 90, 235)

    # kick harness headers
    for j, (ref, names) in enumerate((("J9", ("IN_P1B4", "IN_P1B5", "IN_P1B6")),
                                      ("J10", ("IN_P2B4", "IN_P2B5", "IN_P2B6")))):
        w = sh.place("Connector_Generic", "Conn_02x05_Odd_Even", ref,
                     "KICK-%s" % ("P1" if j == 0 else "P2"), 200, 210 + j * 40,
                     "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical")
        m = {"1": names[0], "3": names[1], "5": names[2]}
        for num, (x, y, ang) in w.items():
            la = 180 if ang == 0 else 0
            if num in m: sh.glabel(m[num], x, y, la)
            elif num in ("2", "4", "6", "8", "10"): sh.pwr("GND", x, y)
            else: sh.nc(x, y)
    # AUX header
    w = sh.place("Connector", "Conn_01x06_Pin", "J11", "AUX", 200, 300,
                 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical")
    for i in range(1, 6):
        sh.glabel("IN_AUX%d" % i, *w[str(i)][:2], 0)
    sh.pwr("GND", *w["6"][:2])

    # 74HC165 chain + RC + pullups
    rn, cn = 301, 300
    for ci, sigs in enumerate(CHAIN):
        ux, uy = 320, 60 + ci * 90
        w = sh.place("74xx", "74LS165", "U%d" % (300 + ci), "74HC165",
                     ux, uy, "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm")
        pin_for = {0: "11", 1: "12", 2: "13", 3: "14",
                   4: "3", 5: "4", 6: "5", 7: "6"}   # A..H
        # per input: series R + C + net
        for bi, sig in enumerate(sigs):
            px, py, pang = w[pin_for[bi]]
            node = "n_%d_%d" % (ci, bi)
            sh.llabel(node, px, py, 180 if pang == 0 else 0)
            rx = ux - 60 - (bi % 2) * 14
            ry = uy - 40 + bi * 9
            wr = sh.place("Device", "R", "R%d" % rn, "1k", rx, ry,
                          "Resistor_SMD:R_0603_1608Metric"); rn += 1
            sh.glabel(sig, *wr["1"][:2], 90)
            sh.llabel(node, *wr["2"][:2], 0)
            wc = sh.place("Device", "C_Small", "C%d" % cn, "100nF",
                          rx + 6, ry + 2, "Capacitor_SMD:C_0603_1608Metric"); cn += 1
            sh.llabel(node, *wc["1"][:2], 0)
            sh.pwr("GND", *wc["2"][:2])
        # pull-up network (bussed, common pin 1 -> 3V3)
        wn = sh.place("Device", "R_Network08", "RN%d" % (300 + ci), "10k",
                      ux + 55, uy, "Resistor_THT:R_Array_SIP9")
        sh.pwr("+3.3V", *wn["1"][:2])
        for bi in range(8):
            sh.llabel("n_%d_%d" % (ci, bi), *wn[str(bi + 2)][:2], 0)
        # control pins
        sh.glabel("IN_LATCH", *w["1"][:2], 180)     # SH/LD
        sh.glabel("IN_CLK",   *w["2"][:2], 180)     # CLK
        sh.pwr("GND", *w["15"][:2])                 # CLK INH
        if ci == 0:
            sh.pwr("GND", *w["10"][:2])             # SER of first chip
        else:
            sh.glabel("CH%d" % ci, *w["10"][:2], 180)
        if ci < 3:
            sh.glabel("CH%d" % (ci + 1), *w["9"][:2], 0)   # QH -> next SER
        else:
            sh.glabel("IN_DATA", *w["9"][:2], 0)           # QH -> MCU
        sh.nc(*w["7"][:2])                          # /QH
        sh.pwr("+3.3V", *w["16"][:2]); sh.pwr("GND", *w["8"][:2])
        wc = sh.place("Device", "C", "C%d" % cn, "100nF", ux + 30, uy + 25,
                      "Capacitor_SMD:C_0603_1608Metric"); cn += 1
        sh.pwr("+3.3V", *wc["1"][:2]); sh.pwr("GND", *wc["2"][:2])
    sh.note("Chain order: U300 shifts first (P1), U303 last (aux).\\n"
            "Poll at 1 kHz: latch low pulse, then 32 clocks on IN_CLK, "
            "read IN_DATA.", 300, 420, 1.8)
    return sh.emit(FILE_UUIDS["inputs"])

# =====================================================================
def gen_power():
    sh = Sheet("power", paper="A4")
    sh.note("5V entry: barrel (bench) or JAMMA (cab) via JP400. Never both.\\n"
            "12V only feeds the (DNP) speaker amp via pi-filter "
            "fc=1/(2*pi*sqrt(10uH*220uF))=3.4kHz, damped by electrolytic ESR.\\n"
            "CHASSIS-GND: 1M || 10nF hybrid + JP402 hard bond (close at ONE "
            "point only; open when JAMMA PSU already bonds earth).", 15, 20, 1.5)
    w = sh.place("Connector", "Barrel_Jack_Switch", "J12", "5V-DC", 40, 60,
                 "Connector_BarrelJack:BarrelJack_Horizontal")
    sh.llabel("vin_raw", *w["1"][:2], 0)
    sh.pwr("GND", *w["2"][:2])
    if "3" in w: sh.pwr("GND", *w["3"][:2])
    w = sh.place("Device", "Polyfuse", "F400", "2A", 75, 60,
                 "Fuse:Fuse_1812_4532Metric")
    sh.llabel("vin_raw", *w["1"][:2], 90)
    sh.llabel("v5_prot", *w["2"][:2], 0)
    w = sh.place("Device", "D_TVS", "D400", "SMBJ5.0A", 95, 70,
                 "Diode_SMD:D_SMB")
    sh.llabel("v5_prot", *w["1"][:2], 90)
    sh.pwr("GND", *w["2"][:2])
    # jumper selects barrel vs jamma into +5V
    w = sh.place("Jumper", "SolderJumper_2_Bridged", "JP400", "BARREL-5V",
                 115, 60, "Jumper:SolderJumper-2_P1.3mm_Bridged_RoundedPad1.0x1.5mm")
    sh.llabel("v5_prot", *w["1"][:2], 180)
    sh.pwr("+5V", *w["2"][:2])
    w = sh.place("Jumper", "SolderJumper_2_Open", "JP401", "JAMMA-5V",
                 115, 75, "Jumper:SolderJumper-2_P1.3mm_Open_TrianglePad1.0x1.5mm")
    sh.glabel("JAMMA_5V", *w["1"][:2], 180)
    sh.pwr("+5V", *w["2"][:2])
    for i, v in enumerate(("100uF", "10uF", "100nF")):
        fp = ("Capacitor_SMD:CP_Elec_6.3x7.7" if v == "100uF" else
              "Capacitor_SMD:C_0805_2012Metric" if v == "10uF" else
              "Capacitor_SMD:C_0603_1608Metric")
        w = sh.place("Device", "C", "C%d" % (400 + i), v, 140 + i * 14, 70, fp)
        sh.pwr("+5V", *w["1"][:2]); sh.pwr("GND", *w["2"][:2])
    sh.flag(190, 63.19); sh.pwr("+5V", 190, 63.19)
    sh.flag(200, 70); sh.pwr("GND", 200, 70)

    # 12V amp rail (DNP)
    w = sh.place("Device", "Polyfuse", "F401", "2A", 60, 120,
                 "Fuse:Fuse_1812_4532Metric", dnp=True)
    sh.glabel("JAMMA_12V", *w["1"][:2], 90)
    sh.llabel("v12_f", *w["2"][:2], 0)
    w = sh.place("Device", "C", "C403", "220uF", 75, 130,
                 "Capacitor_SMD:CP_Elec_8x10", dnp=True)
    sh.llabel("v12_f", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Device", "L", "L400", "10uH 3A", 95, 120, "", dnp=True)
    sh.llabel("v12_f", *w["1"][:2], 90)
    sh.llabel("v12_amp", *w["2"][:2], 0)
    w = sh.place("Device", "C", "C404", "220uF", 115, 130,
                 "Capacitor_SMD:CP_Elec_8x10", dnp=True)
    sh.llabel("v12_amp", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Device", "C", "C405", "10uF", 130, 130,
                 "Capacitor_SMD:C_1206_3216Metric", dnp=True)
    sh.llabel("v12_amp", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    sh.note("Standard-ESR electrolytics on purpose: ESR damps the LC "
            "(Z0=0.21R, zeta=ESR/2Z0).", 55, 145)

    # chassis hybrid ground
    w = sh.place("Device", "R", "R400", "1M", 60, 175,
                 "Resistor_SMD:R_0805_2012Metric")
    sh.glabel("CHASSIS", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Device", "C", "C406", "10nF/250V", 75, 175,
                 "Capacitor_SMD:C_1206_3216Metric")
    sh.glabel("CHASSIS", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    w = sh.place("Jumper", "SolderJumper_2_Open", "JP402", "EARTH-BOND",
                 95, 175, "Jumper:SolderJumper-2_P1.3mm_Open_TrianglePad1.0x1.5mm")
    sh.glabel("CHASSIS", *w["1"][:2], 180); sh.pwr("GND", *w["2"][:2])
    sh.note("CHASSIS: DE15 shell, SD shield, one M3 mounting hole "
            "(star washer).", 55, 190)
    return sh.emit(FILE_UUIDS["power"])

# =====================================================================
def gen_audio():
    """Rebuild audio sheet: PCM5102A application circuit per datasheet,
    keeping the project's symbol and filter values. The previous draft had
    OUTL/OUTR tied to ground symbols and I2S on the wrong pins."""
    import copy
    old = read_file(os.path.join(PROJ, "audio.kicad_sch"))
    m = re.search(r'\(uuid "([0-9a-f-]+)"\)', old)
    file_uuid = m.group(1)
    pcmdef = find_block(old, '(symbol "Audio:PCM5102A"')
    pcmpins = []
    for pm in re.finditer(
        r'\(pin \w+ \w+\s*\(at (-?[\d.]+) (-?[\d.]+) (\d+)\)[\s\S]*?\(name "([^"]+)"[\s\S]*?\(number "([^"]+)"', pcmdef):
        pcmpins.append((1, pm.group(5), float(pm.group(1)), float(pm.group(2)),
                        int(pm.group(3)), pm.group(4)))
    sh = Sheet("audio")
    sh.pwr_n = 550
    # inject harvested def manually
    sh.defs["Audio:PCM5102A"] = pcmdef

    sx, sy = snap(110), snap(80)
    world = {}
    names = {}
    for (_u, num, px, py, ang, nm) in pcmpins:
        world[num] = (round(sx + px, 2), round(sy - py, 2), ang)
        names[nm] = num
    sh.items.append(SYM_T.format(
        lib="Audio", sym="PCM5102A", x=sx, y=sy, ref="U3", val="PCM5102A",
        fp="Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm", unit=1, uuid=U(), dnp="no",
        path="/%s/%s" % (ROOT_UUID, SHEET_UUIDS["audio"])))
    def pin(nm): return world[names[nm]]
    def at(nm, f, *a):
        x, y, ang = pin(nm)
        f(*a, x, y) if f in (sh.pwr,) else None
    # I2S inputs
    for nm, net in (("BCK", "I2S_CK"), ("DIN", "I2S_SDO"), ("LRCK", "I2S_WS")):
        x, y, ang = pin(nm)
        sh.glabel(net, x, y, 180 if ang == 0 else 0)
    # straps
    for nm in ("SCK", "FLT", "DEMP", "FMT"):
        x, y, ang = pin(nm)
        sh.pwr("GND", x, y)
    # XSMT
    x, y, ang = pin("XSMT")
    sh.glabel("XSMT", x, y, 180 if ang == 0 else 0)
    w = sh.place("Device", "R_US", "R3", "10k", 45, 120,
                 "Resistor_SMD:R_0603_1608Metric")
    sh.glabel("XSMT", *w["1"][:2], 90)
    sh.pwr("GND", *w["2"][:2])
    # power pins
    for nm, net in (("CPVDD", "+3.3V"), ("DVDD", "+3.3V"), ("AVDD", "+3.3VA"),
                    ("CPGND", "GND"), ("DGND", "GND"), ("AGND", "GNDA")):
        x, y, ang = pin(nm)
        sh.pwr(net, x, y)
    # charge pump + LDO caps (local labels)
    x, y, ang = pin("CAPP"); sh.llabel("capp", x, y, 180 if ang == 0 else 0)
    x, y, ang = pin("CAPM"); sh.llabel("capm", x, y, 180 if ang == 0 else 0)
    w = sh.place("Device", "C", "C1", "2.2uF", 45, 60,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.llabel("capp", *w["1"][:2], 90); sh.llabel("capm", *w["2"][:2], 270)
    x, y, ang = pin("VNEG"); sh.llabel("vneg", x, y, 180 if ang == 0 else 0)
    w = sh.place("Device", "C", "C2", "2.2uF", 60, 60,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.llabel("vneg", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    x, y, ang = pin("LDOO"); sh.llabel("ldoo", x, y, 0 if ang == 180 else 180)
    w = sh.place("Device", "C", "C3", "2.2uF", 75, 60,
                 "Capacitor_SMD:C_0603_1608Metric")
    sh.llabel("ldoo", *w["1"][:2], 90); sh.pwr("GND", *w["2"][:2])
    # output filters: OUTx -> 470R -> Lx node; 2.2nF node->GNDA; -> jack
    for nm, rref, cref, net, yy in (("OUTL", "R1", "C4", "LC", 55),
                                    ("OUTR", "R2", "C5", "RC", 66)):
        x, y, ang = pin(nm)
        sh.llabel(nm.lower(), x, y, 0 if ang == 180 else 180)
        w = sh.place("Device", "R_US", rref, "470", 150, yy + 25,
                     "Resistor_SMD:R_0603_1608Metric")
        sh.llabel(nm.lower(), *w["1"][:2], 90)
        sh.glabel(net, *w["2"][:2], 270)
        w = sh.place("Device", "C", cref, "2.2nF", 165, yy + 32,
                     "Capacitor_SMD:C_0603_1608Metric")
        sh.glabel(net, *w["1"][:2], 90)
        sh.pwr("GNDA", *w["2"][:2])
    # decoupling bank
    for i, (v, net, gnet) in enumerate((("10uF", "+3.3V", "GND"),
                                        ("100nF", "+3.3V", "GND"),
                                        ("100nF", "+3.3V", "GND"),
                                        ("10uF", "+3.3VA", "GNDA"),
                                        ("100nF", "+3.3VA", "GNDA"))):
        w = sh.place("Device", "C", "C%d" % (6 + i), v, 45 + i * 13, 145,
                     "Capacitor_SMD:C_0805_2012Metric" if v == "10uF"
                     else "Capacitor_SMD:C_0603_1608Metric")
        sh.pwr(net, *w["1"][:2]); sh.pwr(gnet, *w["2"][:2])
    # analog rail bead + ground tie + flags
    w = sh.place("Device", "FerriteBead", "FB1", "600R@100MHz", 130, 145,
                 "Inductor_SMD:L_0805_2012Metric")
    sh.pwr("+3.3V", *w["1"][:2]); sh.pwr("+3.3VA", *w["2"][:2])
    sh.flag(*w["2"][:2])
    w = sh.place("Device", "NetTie_2", "NT1", "NetTie_2", 150, 145,
                 "NetTie:NetTie-2_SMD_Pad0.5mm")
    sh.pwr("GND", *w["1"][:2]); sh.pwr("GNDA", *w["2"][:2])
    sh.flag(*w["2"][:2])
    # line-out jack
    w = sh.place("Connector_Audio", "AudioJack3", "J6", "LINE-OUT", 190, 90,
                 "Connector_Audio:Jack_3.5mm_CUI_SJ1-3533NG_Horizontal")
    sh.pwr("GNDA", *w["S"][:2])
    sh.glabel("LC", *w["T"][:2], 0)
    sh.glabel("RC", *w["R"][:2], 0)
    sh.note("PCM5102A: SCK=GND -> internal PLL from BCK (no MCLK needed).\n"
            "FMT=GND I2S, FLT=GND normal filter, DEMP=GND off.\n"
            "XSMT: 10k pulldown = muted at boot; PD4 drives high to unmute "
            "(pop-free start).\n"
            "Output RC 470R+2.2nF: fc=154 kHz (-38 dB at 384fs).\n"
            "AVDD via FB1 bead; AGND via NT1 net-tie (single ground plane).",
            40, 175, 1.5)
    return sh.emit(file_uuid)

def patch_root():
    p = os.path.join(PROJ, "ArcadePortenta.kicad_sch")
    txt = read_file(p)
    if "power.kicad_sch" in txt:
        print("root: already patched"); return
    blocks = []
    for i, (name, page, x) in enumerate((("power", "5", 33.02),
                                         ("inputs", "6", 93.98))):
        blocks.append('''\t(sheet
\t\t(at %s 70.0)
\t\t(size 52.07 31.75)
\t\t(exclude_from_sim no)
\t\t(in_bom yes)
\t\t(on_board yes)
\t\t(dnp no)
\t\t(fields_autoplaced yes)
\t\t(stroke
\t\t\t(width 0.1524)
\t\t\t(type solid)
\t\t)
\t\t(fill
\t\t\t(color 0 0 0 0.0000)
\t\t)
\t\t(uuid "%s")
\t\t(property "Sheetname" "%s"
\t\t\t(at %s 69.3 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(justify left bottom)
\t\t\t)
\t\t)
\t\t(property "Sheetfile" "%s.kicad_sch"
\t\t\t(at %s 102.3 0)
\t\t\t(effects
\t\t\t\t(font
\t\t\t\t\t(size 1.27 1.27)
\t\t\t\t)
\t\t\t\t(justify left top)
\t\t\t)
\t\t)
\t\t(instances
\t\t\t(project "ArcadePortenta"
\t\t\t\t(path "/%s"
\t\t\t\t\t(page "%s")
\t\t\t\t)
\t\t\t)
\t\t)
\t)
''' % (x, SHEET_UUIDS[name], name, x, name, x, ROOT_UUID, page))
    txt = txt.replace("\t(sheet_instances", "".join(blocks) + "\t(sheet_instances", 1)
    with open(p, "w", encoding="utf-8", newline="\n") as f: f.write(txt)
    print("root: added power + inputs sheets")

# =====================================================================
def main():
    # vga file uuid: reuse existing
    vtxt = read_file(os.path.join(PROJ, "vga.kicad_sch"))
    m = re.search(r'\(uuid "([0-9a-f-]+)"\)', vtxt)
    FILE_UUIDS["vga"] = m.group(1)

    patch_root()
    rtxt = read_file(os.path.join(PROJ, "ArcadePortenta.kicad_sch"))
    for sm in re.finditer(r'\(uuid "([0-9a-f-]+)"\)[\s\S]{0,600}?'
                          r'"Sheetfile" "([a-z]+)\.kicad_sch"', rtxt):
        if sm.group(2) in SHEET_UUIDS:
            SHEET_UUIDS[sm.group(2)] = sm.group(1)

    audio_out = gen_audio()   # reads old file first
    with open(os.path.join(PROJ, "audio.kicad_sch"), "w",
              encoding="utf-8", newline="\n") as f:
        f.write(audio_out)
    print("wrote audio.kicad_sch (%d bytes)" % len(audio_out))
    for name, gen in (("portenta", gen_portenta), ("vga", gen_vga),
                      ("inputs", gen_inputs), ("power", gen_power)):
        out = gen()
        with open(os.path.join(PROJ, name + ".kicad_sch"), "w",
                  encoding="utf-8", newline="\n") as f:
            f.write(out)
        print("wrote %s.kicad_sch (%d bytes)" % (name, len(out)))

if __name__ == "__main__":
    main()
