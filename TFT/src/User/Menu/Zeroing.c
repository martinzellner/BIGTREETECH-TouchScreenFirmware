#include "Zeroing.h"
#include "includes.h"

#define PROBE_MAX_TRAVEL_MM  100.0f
#define PROBE_FEEDRATE       180
#define PROBE_RETRACT_MM     10.0f
#define PROBE_RETRACT_SPEED  480

static char plateLabel[16];
static char tcXyLabel[18];
static char tcZLabel[14];
static char probeConfirmMsg[112];

static void runZProbe(void)
{
  float plate = infoSettings.touch_plate_thickness;

  // Safe LowRider 4 downward Z probe:
  // 1) Relative mode + slow controlled probe down (never inherits rapid feedrate)
  // 2) Return to absolute mode and set Z to configured touch plate thickness
  // 3) Retract Z safely above touch plate so clip & plate can be removed
  storeCmd("G91\n");
  storeCmd("G38.2 Z-%.1f F%d\n", PROBE_MAX_TRAVEL_MM, PROBE_FEEDRATE);
  storeCmd("G90\n");
  storeCmd("G92 Z%.2f\n", plate);
  storeCmd("G0 Z%.2f F%d\n", plate + PROBE_RETRACT_MM, PROBE_RETRACT_SPEED);
  addToast(DIALOG_TYPE_INFO, "Probing Z...");
}

static void moveToToolChangePos(void)
{
  // Stop spindle, lift Z safely in machine coords, then move to front-access TC XY
  // G53 preserves G92 workpiece X0/Y0!
  storeCmd("M05\n");
  storeCmd("G90\n");
  storeCmd("G53 G0 Z%d F480\n", infoSettings.tool_change_pos[Z_AXIS]);
  storeCmd("G53 G0 X%d Y%d F2100\n", infoSettings.tool_change_pos[X_AXIS], infoSettings.tool_change_pos[Y_AXIS]);
  addToast(DIALOG_TYPE_INFO, "Moving to Tool Change...");
}

static void liftZInPlace(void)
{
  // In-place quick tool swap without moving XY (works even if not homed)
  storeCmd("M05\n");
  storeCmd("G91\n");
  storeCmd("G0 Z60 F480\n");
  storeCmd("G90\n");
  addToast(DIALOG_TYPE_INFO, "Spindle OFF, Z +60mm");
}

static void goWorkXYZeroFromTC(void)
{
  storeCmd("G91\n");
  storeCmd("G0 Z15 F480\n");
  storeCmd("G90\n");
  storeCmd("G0 X0 Y0 F2100\n");
  addToast(DIALOG_TYPE_INFO, "Moving to Work X0 Y0");
}

void menuToolChange(void)
{
  sprintf(tcXyLabel, "X%d Y%d", infoSettings.tool_change_pos[X_AXIS], infoSettings.tool_change_pos[Y_AXIS]);
  sprintf(tcZLabel, "TC Z%d", infoSettings.tool_change_pos[Z_AXIS]);

  MENUITEMS tcItems = {
    // title
    {.address = "Tool Change"},
    // icon                          label
    {
      {ICON_LOAD,                    {.address = "Move to TC"}},
      {ICON_UNLOAD,                  {.address = "Lift Z+60"}},
      {ICON_BLTOUCH_DEPLOY,          LABEL_PROBING_Z},
      {ICON_BLTOUCH_TEST,            {.address = "Probe Test"}},
      {ICON_MOVE,                    {.address = "Go XY0"}},
      {ICON_PARAMETER,               {.address = tcXyLabel}},
      {ICON_Z_INC,                   {.address = tcZLabel}},
      {ICON_BACK,                    LABEL_BACK},
    }
  };

  KEY_VALUES key_num = KEY_IDLE;

  menuDrawPage(&tcItems);

  while (MENU_IS(menuToolChange))
  {
    key_num = menuKeyGetValue();
    switch (key_num)
    {
      case KEY_ICON_0:
        sprintf(probeConfirmMsg,
                "Stop spindle (M05) & move to machine\nX%d Y%d Z%d? (Keeps Work XY0)",
                infoSettings.tool_change_pos[X_AXIS],
                infoSettings.tool_change_pos[Y_AXIS],
                infoSettings.tool_change_pos[Z_AXIS]);
        popupDialog(DIALOG_TYPE_QUESTION, (uint8_t *)"Tool Change", (uint8_t *)probeConfirmMsg,
                    LABEL_CONFIRM, LABEL_CANCEL, moveToToolChangePos, NULL, NULL);
        break;

      case KEY_ICON_1:
        popupDialog(DIALOG_TYPE_QUESTION, (uint8_t *)"Quick Swap",
                    (uint8_t *)"Stop spindle (M05) & lift Z +60mm in place?",
                    LABEL_CONFIRM, LABEL_CANCEL, liftZInPlace, NULL, NULL);
        break;

      case KEY_ICON_2:
        sprintf(probeConfirmMsg,
                "Attach clip & place %.2fmm touch plate under new bit.\nRe-probe Z?",
                infoSettings.touch_plate_thickness);
        popupDialog(DIALOG_TYPE_ALERT, LABEL_PROBING_Z, (uint8_t *)probeConfirmMsg,
                    LABEL_CONFIRM, LABEL_CANCEL, runZProbe, NULL, NULL);
        break;

      case KEY_ICON_3:
        storeCmd("M119\n");
        OPEN_MENU(menuTerminal);
        break;

      case KEY_ICON_4:
        popupDialog(DIALOG_TYPE_QUESTION, (uint8_t *)"Go XY0",
                    (uint8_t *)"Lift Z +15mm and return to Work X0 Y0?",
                    LABEL_CONFIRM, LABEL_CANCEL, goWorkXYZeroFromTC, NULL, NULL);
        break;

      case KEY_ICON_5:
      {
        int32_t nx = numPadInt((uint8_t *)"TC Machine X (mm)", infoSettings.tool_change_pos[X_AXIS], TOOL_CHANGE_X_POS, false);
        int32_t ny = numPadInt((uint8_t *)"TC Machine Y (mm)", infoSettings.tool_change_pos[Y_AXIS], TOOL_CHANGE_Y_POS, false);
        if (nx < 0) nx = 0;
        if (nx > infoSettings.machine_size_max[X_AXIS]) nx = infoSettings.machine_size_max[X_AXIS];
        if (ny < 0) ny = 0;
        if (ny > infoSettings.machine_size_max[Y_AXIS]) ny = infoSettings.machine_size_max[Y_AXIS];
        infoSettings.tool_change_pos[X_AXIS] = (int16_t)nx;
        infoSettings.tool_change_pos[Y_AXIS] = (int16_t)ny;
        saveSettings();
        sprintf(tcXyLabel, "X%d Y%d", infoSettings.tool_change_pos[X_AXIS], infoSettings.tool_change_pos[Y_AXIS]);
        menuDrawPage(&tcItems);
        break;
      }

      case KEY_ICON_6:
      {
        int32_t nz = numPadInt((uint8_t *)"TC Machine Z (mm)", infoSettings.tool_change_pos[Z_AXIS], TOOL_CHANGE_Z_POS, false);
        if (nz < 0) nz = 0;
        if (nz > infoSettings.machine_size_max[Z_AXIS]) nz = infoSettings.machine_size_max[Z_AXIS];
        infoSettings.tool_change_pos[Z_AXIS] = (int16_t)nz;
        saveSettings();
        sprintf(tcZLabel, "TC Z%d", infoSettings.tool_change_pos[Z_AXIS]);
        menuDrawPage(&tcItems);
        break;
      }

      case KEY_ICON_7:
        CLOSE_MENU();
        break;

      default:
        break;
    }

    loopProcess();
  }
}

void menuZeroing(void)
{
  sprintf(plateLabel, "%.2fmm", infoSettings.touch_plate_thickness);

  MENUITEMS zeroItems = {
    // title
    LABEL_ZEROING,
    // icon                          label
    {
      {ICON_ZEROING,                 LABEL_ZEROING},
      {ICON_X_ZEROING,               LABEL_X},
      {ICON_Y_ZEROING,               LABEL_Y},
      {ICON_Z_ZEROING,               LABEL_Z},
      {ICON_BLTOUCH_DEPLOY,          LABEL_PROBING_Z},
      {ICON_BLTOUCH_TEST,            {.address = "Probe Test"}},
      {ICON_PARAMETER,               {.address = plateLabel}},
      {ICON_BACK,                    LABEL_BACK},
    }
  };

  KEY_VALUES key_num = KEY_IDLE;

  menuDrawPage(&zeroItems);

  while (MENU_IS(menuZeroing))
  {
    key_num = menuKeyGetValue();
    switch (key_num)
    {
      case KEY_ICON_0:
        storeCmd("G92 X0 Y0 Z0\n");
        addToast(DIALOG_TYPE_INFO, "Work XYZ = 0");
        break;

      case KEY_ICON_1:
        storeCmd("G92 X0\n");
        addToast(DIALOG_TYPE_INFO, "Work X = 0");
        break;

      case KEY_ICON_2:
        storeCmd("G92 Y0\n");
        addToast(DIALOG_TYPE_INFO, "Work Y = 0");
        break;

      case KEY_ICON_3:
        storeCmd("G92 Z0\n");
        addToast(DIALOG_TYPE_INFO, "Work Z = 0");
        break;

      case KEY_ICON_4:
        sprintf(probeConfirmMsg,
                "Attach clip & place %.2fmm touch plate under bit.\nStart slow Z probe?",
                infoSettings.touch_plate_thickness);
        popupDialog(DIALOG_TYPE_ALERT, LABEL_PROBING_Z, (uint8_t *)probeConfirmMsg,
                    LABEL_CONFIRM, LABEL_CANCEL, runZProbe, NULL, NULL);
        break;

      case KEY_ICON_5:
        // Continuity test: send M119 and open Terminal to verify z_min OPEN / TRIGGERED
        storeCmd("M119\n");
        OPEN_MENU(menuTerminal);
        break;

      case KEY_ICON_6:
      {
        float val = (float)numPadFloat((uint8_t *)"Touch Plate (mm)",
                                       infoSettings.touch_plate_thickness,
                                       TOUCH_PLATE_THICKNESS, false);
        if (val < 0.0f) val = 0.0f;
        if (val > 50.0f) val = 50.0f;
        infoSettings.touch_plate_thickness = val;
        saveSettings();
        sprintf(plateLabel, "%.2fmm", infoSettings.touch_plate_thickness);
        menuDrawPage(&zeroItems);
        break;
      }

      case KEY_ICON_7:
        CLOSE_MENU();
        break;

      default:
        break;
    }

    loopProcess();
  }
}
