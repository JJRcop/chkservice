#include "chk.h"
#include "chk-ui.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow() {
  resize();
  std::signal(SIGWINCH, MainWindow::sigwinch);
  padding->x = 2;
  padding->y = 2;
}

MainWindow::~MainWindow() {
  delwin(win);
}

void MainWindow::sigwinch(int c) {
  stopCurses();
  exit(0);
}

void MainWindow::createMenu() {
  win = newwin(screenSize->h, screenSize->w, 0, 0);
  keypad(win, true);
  keypad(stdscr, true);

  while(1) {
    drawUnits();

    int key = wgetch(win);
    error(NULL);

    switch(key) {
      case 'k':
      case KEY_UP:
        moveUp();
        break;
      case 'j':
      case KEY_DOWN:
        moveDown();
        break;
      case 'f':
      case KEY_NPAGE:
        movePageDown();
        break;
      case 'b':
      case KEY_PPAGE:
        movePageUp();
        break;
      case 'q':
        stopCurses();
        delwin(win);
        exit(0);
        break;
      case ' ':
        toggleUnitState();
        break;
      case 's':
        toggleUnitSubState();
        break;
      case 'r':
        updateUnits();
        drawUnits();
        error((char *)"Updated..");
        break;
      case '?':
        aboutWindow(screenSize);
        break;
      default:
        break;
    }
  }
}

void MainWindow::resize() {
  getmaxyx(stdscr, screenSize->h, screenSize->w);
}

void MainWindow::moveUp() {
  int ps = winSize->h - (padding->y + 1);

  if (start > 0 && selected < ps / 2) {
    start--;
  } else if (selected > 0) {
    selected--;
  }

  if (units[start + selected]->id.size() == 0) {
    moveUp();
  }
}

void MainWindow::moveDown() {
  int offset = start + selected;
  int ps = winSize->h - (padding->y + 1);
  int max = units.size() - 1;

  if ((start + ps) < max) {
    if (selected < ps / 2) {
      selected++;
    } else {
      start++;
    }
  } else if (offset < max) {
    selected++;
  }

  if (offset >= max) {
    selected = ps;
  }

  if (units[start + selected]->id.size() == 0) {
    moveDown();
  }
}

void MainWindow::movePageUp() {
  int ps = winSize->h - (padding->y + 1);

  if (start > 0) {
    start -= ps;
  }

  if (start < 0) {
    start = 0;
    selected = 0;
  }

  if (units[start + selected]->id.size() == 0) {
    moveUp();
  }
}

void MainWindow::movePageDown() {
  int ps = winSize->h - 3;
  int max = units.size() - 1;

  if ((start + ps / 2) < max) {
    start += ps;
  }

  if ((start + ps) > max) {
    start = max - ps;
    selected = ps;
  }

  if (units[start + selected]->id.size() == 0) {
    moveDown();
  }
}

void MainWindow::updateUnits() {
  units.clear();

  try {
    ctl->fetch();
    units = ctl->getItemsSorted();
  } catch(std::string &err) {
    error((char *)err.c_str());
    return;
  }
}

void MainWindow::drawUnits() {
  if (units.empty()) {
    updateUnits();
  }

  getmaxyx(win, winSize->h, winSize->w);
  winSize->h -= padding->y;

  for (int i = 0; i < (winSize->h - padding->y); i++) {
    if ((i + start) > (int)units.size() - 1) {
      break;
    }

    UnitItem *unit = units[start + i];

    if (i == selected) {
      wattron(win, A_REVERSE);
    }

    drawItem(unit, i + padding->y);
    wattroff(win, A_REVERSE);
  }

  drawInfo();

  refresh();
  wrefresh(win);
}

void MainWindow::drawItem(UnitItem *unit, int y) {
  if (unit->id.size() == 0) {
    if (unit->target.size() == 0) {
      printInMiddle(win, y, 0, winSize->w, (char *)"", COLOR_PAIR(3), (char *)' ');
    } else {
      std::string title(unit->target);
      title += "s";
      title[0] = std::toupper(title[0]);

      printInMiddle(win, y, 0, winSize->w, (char *)"", COLOR_PAIR(3), (char *)' ');
      printInMiddle(win, y, 0, winSize->w / 2, (char *)title.c_str(), COLOR_PAIR(3), (char *)' ');
    }
    return;
  }

  if (unit->state == UNIT_STATE_ENABLED) {
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, y, padding->x, "[x]");
    wattroff(win, COLOR_PAIR(2));
  } else if (unit->state == UNIT_STATE_DISABLED) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x, "[ ]");
    wattroff(win, COLOR_PAIR(5));
  } else if (unit->state == UNIT_STATE_STATIC) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x, "[s]");
    wattroff(win, COLOR_PAIR(5));
  } else if (unit->state == UNIT_STATE_BAD) {
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, y, padding->x, "-b-");
    wattroff(win, COLOR_PAIR(1));
  } else if (unit->state == UNIT_STATE_MASKED) {
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, y, padding->x, "-m-");
    wattroff(win, COLOR_PAIR(3));
  }

  if (unit->sub == UNIT_SUBSTATE_RUNNING) {
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, y, padding->x + 3, "  >  ");
    wattroff(win, COLOR_PAIR(3));
  } else if (unit->state == UNIT_SUBSTATE_CONNECTED) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x + 3, "  =  ");
    wattroff(win, COLOR_PAIR(5));
  } else {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x + 3, "     ");
    wattroff(win, COLOR_PAIR(5));
  }

  unsigned int leftPad = padding->x + 8;
  unsigned int rightPad = (winSize->w - leftPad);

  if (unit->id.size() > (rightPad - padding->x)) {
    unit->id.resize(rightPad - padding->x);
  }

  std::stringstream sline;

  unit->description.resize(winSize->w / 2, ' ');
  sline << std::string(unit->id.size(), ' ') << " "
    << std::setw(rightPad - unit->id.size())
    << unit->description;

  std::string cline(sline.str());
  std::string name(unit->id);

  name.resize(cline.find_first_of(unit->description[0]), ' ');

  if (cline.size() > rightPad) {
    cline.resize(rightPad - 2 );
  }

  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, y, leftPad, "%s", cline.c_str());
  wattroff(win, COLOR_PAIR(4));
  mvwprintw(win, y, leftPad, "%s", name.c_str());
}

void MainWindow::drawInfo() {
  int count = 0;
  int countUntilNow = start + selected;

  for (auto unit : units) {
    if (unit->id.size() == 0) {
      if (countUntilNow > count) {
        countUntilNow--;
      }
      continue;
    }

    count++;
  }

  std::stringstream position;

  position << countUntilNow + 1 << "/" << count;

  printInMiddle(win, winSize->h + 1, 0, winSize->w, (char *)"", COLOR_PAIR(5), (char *)' ');
  printInMiddle(win, winSize->h + 1, 0, (winSize->w / 2), (char *)position.str().c_str(), COLOR_PAIR(5), (char *)NULL);

  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, winSize->h + 1, winSize->w - 10, "? - help");
  wattroff(win, COLOR_PAIR(4));
}

void MainWindow::error(char *err) {
  mvwprintw(win, 0, 0, std::string(winSize->w, ' ').c_str());

  if (err) {
    mvwprintw(win, 0, 1, err);
  }
}

void MainWindow::toggleUnitState() {
  try {
    ctl->toggleUnitState(units[start + selected]);
  } catch (std::string &err) {
    error((char *)err.c_str());
  }
}

void MainWindow::toggleUnitSubState() {
  try {
    ctl->toggleUnitSubState(units[start + selected]);
  } catch (std::string &err) {
    error((char *)err.c_str());
  }
}
