#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
//#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;
QT_END_NAMESPACE

#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QMainWindow>
#include <QTimer>
#include <QToolTip>
#include <QMouseEvent>


// This is crucial: undefine the 'signals' macro after Qt headers
// but *before* any GTK/GLib headers that might use 'signals' as a struct member.
#ifdef signals
#undef signals
#endif


#include "pdef.h"
#include "memgive.h"
#include "imdb.h"
#include "omdb1.h"

class myQTreeView : public QTreeView {
    Q_OBJECT
public:
    myQTreeView(QWidget *parent = nullptr) : QTreeView(parent) {
        setMouseTracking(true); // Enable mouse tracking

        hoverTimer = new QTimer(this);
        hoverTimer->setSingleShot(true);
        hoverTimer->setInterval(1000); // 1-second delay for tooltip
        connect(hoverTimer, &QTimer::timeout, this, &myQTreeView::showToolTip);
    }

    // Keep your existing custom methods and constructors
    using QTreeView::selectionChanged;
    void selectionChanged();
    char* my_tooltip(int row, int col, char *buf);

protected:
    void mouseMoveEvent(QMouseEvent *event) override {
        QModelIndex hoverIndex = indexAt(event->pos());
        if (hoverIndex.isValid()) {
            // Restart the timer every time the mouse moves to a new item
            hoverTimer->start();
            currentHoverIndex = hoverIndex;
        } else {
            hoverTimer->stop();
        }
        QTreeView::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        hoverTimer->stop(); // Stop the timer if the mouse leaves the view
        QTreeView::leaveEvent(event);
    }

private slots:
    void showToolTip();

private:
    QTimer *hoverTimer;
    QModelIndex currentHoverIndex;
};

class myQSortFilterProxyModel : public QSortFilterProxyModel
{
Q_OBJECT
public:
myQTreeView *myQTV;
myQSortFilterProxyModel(myQTreeView *_myQTV):QSortFilterProxyModel() {myQTV=_myQTV;};
using QSortFilterProxyModel::sort;
void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);  // or Qt::DescendingOrder (makes no difference?)

using QSortFilterProxyModel::lessThan;
bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};


struct MYFONT {const char *pnam; signed char ptsz, weight, italic;};

class UserFonts
{
public:
UserFonts();
~UserFonts();
QFont qfcol(int col);
signed char fldfnt[NUMCOLS];
private:
DYNAG *mff;     // table of all MYFONT values used
DYNAG *fontname;
};

class Window : public QWidget
//class Window : public QMainWindow
{
    Q_OBJECT
public:
    Window();
    ~Window() {proxyView->my_tooltip(0,0,0);};
    void setSourceModel(QAbstractItemModel *model);
    myQTreeView *proxyView;                   // public! (previously private)
    void refocus(int32_t focus_imno);
    QGroupBox *proxyGroupBox;          // public so we can set the title from outsde the class
    bool live;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    int32_t get_current_em_key(OMZ *e);

private slots:
    void filterRegularExpressionChanged();
    void filterColumnChanged();
    int filterMatches();
    void sortChanged();
    void webLink();
    void rating1();
    void whereIs();
    void usrTxt();
    void ReStart(bool toggle);     // if toggle=true, swap LIVE/DEV paths in params before restart
    void set_focus(int32_t imno);

private:
    QAction *webLinkAct;
    QAction *rating1Act;
    QAction *whereIsAct;
    QAction *usrTxtAct;
    QAction *ReStartAct;
    QAction *ReStartXAct;          // toggle LIVE/DEV in params before restart

private:
    QSortFilterProxyModel *proxyModel;
    QCheckBox *filterCaseSensitivityCheckBox;
    QCheckBox *sortCaseSensitivityCheckBox;
    QLabel *filterPatternLabel;
    QLabel *filterColumnLabel;
    QLineEdit *filterPatternLineEdit;
    QComboBox *filterColumnComboBox;

    QCheckBox *toggleCheckBox;
    QSlider *precisionSlider;

};

#endif // WINDOW_H
