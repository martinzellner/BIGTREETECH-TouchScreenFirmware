#include "Home.h"
#include "includes.h"

static void homeAllAxes(void)
{
  storeCmd("G28\n");
}

static void goWorkXYZeroSafe(void)
{
  // Lift Z safely before rapid XY travel to avoid hitting clamps or stock
  storeCmd("G91\n");
  storeCmd("G0 Z15 F480\n");
  storeCmd("G90\n");
  storeCmd("G0 X0 Y0 F2100\n");
}

static void parkZForPowerOff(void)
{
  // Lower LR4 gantry gently before power-off so it does not drop onto spoilboard
  storeCmd("G90\n");
  storeCmd("G0 Z10 F480\n");
}

const MENUITEMS homeItems = {
  // title
  LABEL_HOME,
  // icon                          label
  {
    {ICON_HOME,                    LABEL_HOME},
    {ICON_X_HOME,                  LABEL_X},
    {ICON_Y_HOME,                  LABEL_Y},
    {ICON_Z_HOME,                  LABEL_Z},
    {ICON_MOVE,                    {.address = "Go XY0"}},
    {ICON_Z_DEC,                   {.address = "Park Z"}},
    {ICON_LOAD,                    {.address = "Tool Chg"}},
    {ICON_BACK,                    LABEL_BACK},
  }
};

void menuHome(void)
{
  KEY_VALUES key_num = KEY_IDLE;

  menuDrawPage(&homeItems);

  while (MENU_IS(menuHome))
  {
    key_num = menuKeyGetValue();
    switch (key_num)
    {
      case KEY_ICON_0:
        popupDialog(DIALOG_TYPE_ALERT, LABEL_HOME,
                    (uint8_t *)"Home all axes (G28)?\nSquares Y/Z & resets work zero.",
                    LABEL_CONFIRM, LABEL_CANCEL, homeAllAxes, NULL, NULL);
        break;

      case KEY_ICON_1: storeCmd("G28 X\n"); break;
      case KEY_ICON_2: storeCmd("G28 Y\n"); break;
      case KEY_ICON_3: storeCmd("G28 Z\n"); break;

      case KEY_ICON_4:
        popupDialog(DIALOG_TYPE_QUESTION, (uint8_t *)"Go XY0",
                    (uint8_t *)"Lift Z +15mm and move to Work X0 Y0?",
                    LABEL_CONFIRM, LABEL_CANCEL, goWorkXYZeroSafe, NULL, NULL);
        break;

      case KEY_ICON_5:
        popupDialog(DIALOG_TYPE_QUESTION, (uint8_t *)"LR4 Park Z",
                    (uint8_t *)"Lower Z to 10mm for safe power-off?",
                    LABEL_CONFIRM, LABEL_CANCEL, parkZForPowerOff, NULL, NULL);
        break;

      case KEY_ICON_6:
        OPEN_MENU(menuToolChange);
        break;

      case KEY_ICON_7: CLOSE_MENU();      break;
      default: break;
    }

    loopProcess();
  }
}
