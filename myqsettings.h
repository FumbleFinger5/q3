#include <QtWidgets>

// POSSIBLY should only take the parent window param, since it MUST have a QTreeView child
// (this derived QSettings class saves top window Geometry AND ColumnWidths in child view)

class MyQSettings { // Persistent MainWindow Geometry and TreeView ColumnWidths
public:
    MyQSettings(QWidget *parent_window);
    ~MyQSettings();

private:
QWidget *wind;
QSettings *Qs;  // fails with "stack smashing detected" unless pointer + new
};

void get_conf_number(int argc, char *argv[]);
