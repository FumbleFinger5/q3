#include <QtWidgets>
#include <QCoreApplication>
#include <QProcess>
#include <QStringList>
// This is crucial: undefine the 'signals' macro after Qt headers
// but before any GTK/GLib headers that might use 'signals' as a struct member.
#ifdef signals
#undef signals
#endif

#include <string.h>
#include <unistd.h>

#include "pdef.h"
#include "memgive.h"
#include "str.h"
#include "log.h"
#include "cal.h"
#include "smdb.h"
#include "csv.h"
#include "omdb1.h"
#include "exec.h"
#include "mvdb.h"
#include "qblob.h"

#include "window.h"

extern FLDX fld[];


void myQSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
if (column==COL_RECENT) order=(Qt::SortOrder)0;     // force always AscendingOrder for col:1 (Recent)
QSortFilterProxyModel::sort(column,order);
myQTV->selectionChanged();
}

static char *skip_non_alphanumeric(char *a)
{
int i;
for (i=0; a[i]; i++) if (isalnum(a[i])) break;
return(&a[i]);
}

bool myQSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
char *a, astr[100], *b, bstr[100];
strncpy(a=astr,sourceModel()->data(left).toString().toStdString().c_str(),sizeof(astr)-1);
strncpy(b=bstr,sourceModel()->data(right).toString().toStdString().c_str(),sizeof(bstr)-1);
int cmp=0, col;
switch (col=left.column())    // Column numbers model->index(row,COL) as per imdb.cpp:FLDX fld[NUMCOLS]
{
case COL_TITLE:  // If col==MovieName, step a,b past any non-alphanumeric chars
    cmp=stricmp(skip_non_alphanumeric(a), skip_non_alphanumeric(b));
    break;
case COL_RECENT:
    cmp=(left.data(Qt::UserRole).value<int>() - right.data(Qt::UserRole).value<int>());
    break;
case COL_RUNTIME: case COL_TMDB: case COL_IMDB: case COL_GB:
    if (col==COL_RUNTIME) {strip(a,':'); strip(b,':');}     // then fall thru
    if (col==COL_GB) {strip(a,'.'); strip(b,'.');}     // (then fall thru for shared number compare fn)
    cmp=a2l(a,0)-a2l(b,0);
    break;
case COL_DIRECTOR: case COL_CAST:
    cmp=cp_name(a,b);
    break;
case COL_ADDED: case COL_SEEN: case COL_MYSEEN:
    cmp=cal_parse(a)-cal_parse(b);
    if (!cmp && col==COL_SEEN) // User data for the "seen" cell is FULL dttm, not short bd
        cmp=(left.data(Qt::UserRole).value<int>() - right.data(Qt::UserRole).value<int>());
    break;
default:
    cmp=stricmp(a,b);
    }
return(cmp<0);
}

void myQTreeView::selectionChanged()    // Force display back to current line after sorting
{
scrollTo(currentIndex(),QTreeView::PositionAtCenter);   // OR  PositionAtBottom, PositionAtTop, EnsureVisible
}

static inline QColor textColor(const QPalette &palette)
{
return palette.color(QPalette::Active, QPalette::Text);
}

static void setTextColor(QWidget *w, const QColor &c)
{
auto palette = w->palette();
if (textColor(palette) != c) {
    palette.setColor(QPalette::Active, QPalette::Text, c);
    w->setPalette(palette);
    }
}

void Window::refocus(int32_t imno)
{
if (imno==0) return;
QModelIndex rIndex;
for (int ct = proxyModel->rowCount(proxyView->currentIndex().parent()); --ct>=0; )
    if (proxyModel->data(rIndex = proxyModel->index (ct, COL_IMDB, QModelIndex()), 0).toInt()==imno)    // XYZ?
        {
        proxyView->scrollTo(rIndex, QTreeView::PositionAtCenter);
        proxyView->setCurrentIndex(rIndex);
        }
}

char* myQTreeView::my_tooltip(int row, int col, char *buf)
{
static IMDB_API *ia=0;
if (buf==0) {SCRAP(ia); return(0);}    // IF called by containing window destructor, release ptr & finish
if (ia==0) ia=new IMDB_API;
QModelIndex index = model()->index(row, COL_IMDB);
int32_t imno = model()->data(index).toInt();
if (col==COL_RATING)
   {
   const char *rating=ia->get(imno,"imdbRating");
   const char *votes=ia->get(imno,"imdbVotes");
   *buf=0;
   if (*rating || *votes) 
   strfmt(buf,"IMDB Rated %s (%s votes)", rating, votes);
   return(buf);
   }
if (col!=COL_TITLE) {*buf=0; return(buf);}
const char *p=ia->get(imno,"plot");
const char *year=ia->get(imno,"Year");
strfmt(buf,"%s%s  (%s)%s", "<span style='color:green;'>",
       model()->data(model()->index(row, COL_TITLE)).toString().toStdString().c_str(), year, "</span>");
strendfmt(buf,"  %sI:%d%s<br><br>%s", "<span style='color:grey;'>", imno, "</span>", p);
return(buf);
}

void myQTreeView::showToolTip()
{
if (currentHoverIndex.isValid())
    {
    QString itemData;
    int row=currentHoverIndex.row(), col=currentHoverIndex.column();
    char buf8k[8192];
    if (*my_tooltip(row,col,buf8k))
        itemData = buf8k;
    else
        {
        itemData = model()->data(currentHoverIndex).toString();
        itemData.replace(",", "<br>"); // Replace commas with HTML line break
        }
    // Customize the tooltip as needed, for example:
    QString styledToolTip = QString(
                                "<div style='"
                                "background-color: white; "
                                "color: black; "
                                "font-weight: bold; " // This makes the text bold
                                "font-size: 13pt;'>" // Adjust the font size as needed
                                "%1</div>"
                                ).arg(itemData);

    QToolTip::showText(QCursor::pos(), styledToolTip, this);
    }
}


char *cal_build_dttm_str(char *s)	// reformat compile Parses __DATE__ + __TIME__ strings
{
const char *d= __DATE__;
const char *t= __TIME__;
int32_t ctim=cal_build_dttm(d,t);
calfmt(s,"%02T:%02I %3.3M %d",ctim);	//May 31 2024 11:38:41
return(s);
}

static void custom_scrollbar(myQTreeView &pv)
{
pv.setStyleSheet
    (
    "QScrollBar:vertical {background: #f0f0f0; width: 40px;}"  // 6/10/24 - was width 32
    "QScrollBar::handle:vertical {background: #00FF00; min-height: 48px; border-radius: 4px;}"  // Bright GREEN
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {height: 0px;}"
    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {background: none;}"
    );
}


Window::Window()
{
proxyView = new myQTreeView;
proxyModel = new myQSortFilterProxyModel(proxyView);
proxyView->setRootIsDecorated(false);
//proxyView->setAlternatingRowColors(true);
custom_scrollbar(*proxyView);

proxyView->setModel(proxyModel);
proxyView->setSortingEnabled(true);

sortCaseSensitivityCheckBox = new QCheckBox("Case sensitive sorting");
filterCaseSensitivityCheckBox = new QCheckBox("Case sensitive filter");

filterPatternLineEdit = new QLineEdit;

QToolButton *clearButton = new QToolButton(filterPatternLineEdit);
clearButton->setStyleSheet("background-color: red;");  // Adjust button appearance
QHBoxLayout *layout = new QHBoxLayout(filterPatternLineEdit);
layout->addStretch();
layout->addWidget(clearButton);
layout->setSpacing(0);
layout->setContentsMargins(0, 0, 0, 0);
filterPatternLineEdit->setLayout(layout);
clearButton->setVisible(!filterPatternLineEdit->text().isEmpty());
connect(clearButton, &QToolButton::clicked, filterPatternLineEdit, &QLineEdit::clear);
connect(filterPatternLineEdit, &QLineEdit::textChanged, clearButton, [=](const QString &text)
   {clearButton->setVisible(!text.isEmpty()); });

filterPatternLabel = new QLabel("&Filter pattern:");
filterPatternLabel->setBuddy(filterPatternLineEdit);

filterColumnComboBox = new QComboBox;
for (int i=0;i<NUMCOLS;i++)
    filterColumnComboBox->addItem((const char *)fld[i].name);
filterColumnLabel = new QLabel("Filter &column:");
filterColumnLabel->setBuddy(filterColumnComboBox);

connect(filterPatternLineEdit, &QLineEdit::textChanged, this, &Window::filterRegularExpressionChanged);
connect(filterColumnComboBox, &QComboBox::currentTextChanged, this, &Window::filterColumnChanged);
connect(filterCaseSensitivityCheckBox, &QAbstractButton::toggled, this, &Window::filterRegularExpressionChanged);
connect(sortCaseSensitivityCheckBox, &QAbstractButton::toggled, this, &Window::sortChanged);

char wrk[256];
proxyGroupBox = new QGroupBox;
QGridLayout *proxyLayout = new QGridLayout;

proxyLayout->addWidget(proxyView, 0, 0, 1, 3);  // the variable-size grid display itself
proxyLayout->addWidget(filterPatternLabel, 1, 0);
proxyLayout->addWidget(filterPatternLineEdit, 1, 1, 1, 2);
proxyLayout->addWidget(filterColumnLabel, 2, 0);
proxyLayout->addWidget(filterColumnComboBox, 2, 1, 1, 2);
proxyLayout->setContentsMargins(0, 10, 0, 10);  // This removes the left and right margins, but keeps top and bottom at 10 pixels.
proxyGroupBox->setLayout(proxyLayout);

QHBoxLayout *bottomControlsLayout = new QHBoxLayout;
bottomControlsLayout->addWidget(filterCaseSensitivityCheckBox);
bottomControlsLayout->addWidget(sortCaseSensitivityCheckBox);

whereIsAct = new QAction("Which backup disc", this);
connect(whereIsAct, &QAction::triggered, this, &Window::whereIs);

webLinkAct = new QAction("IMDB", this);
connect(webLinkAct, &QAction::triggered, this, &Window::webLink);
if (user_not_steve)
    {
    rating1Act = new QAction("Set User Rating", this);
    connect(rating1Act, &QAction::triggered, this, &Window::rating1);
    }

usrTxtAct = new QAction("Notes", this);
connect(usrTxtAct, &QAction::triggered, this, &Window::usrTxt);
ReStartAct = new QAction("Restart", this);
connect(ReStartAct, &QAction::triggered, this, [this]{ ReStart(false); });
ReStartXAct = new QAction(running_live?"Restart DEV":"Restart LIVE", this);
connect(ReStartXAct, &QAction::triggered, this, [this]{ ReStart(true); });

QVBoxLayout *mainLayout = new QVBoxLayout;
mainLayout->addWidget(proxyGroupBox);
mainLayout->addLayout(bottomControlsLayout);
setLayout(mainLayout);
strfmt(wrk,"Movie Library - Built: ");
cal_build_dttm_str(strend(wrk));
setWindowTitle(wrk);

filterColumnComboBox->setCurrentIndex(COL_TITLE);
filterCaseSensitivityCheckBox->setChecked(false);
sortCaseSensitivityCheckBox->setChecked(false);
}

void Window::setSourceModel(QAbstractItemModel *model)
{proxyModel->setSourceModel(model);}

void Window::filterRegularExpressionChanged()
{
QString pattern = filterPatternLineEdit->text();
QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
if (!filterCaseSensitivityCheckBox->isChecked())
    options |= QRegularExpression::CaseInsensitiveOption;
QRegularExpression regularExpression(pattern, options);
if (regularExpression.isValid())
    {
    filterPatternLineEdit->setToolTip(QString());
    proxyModel->setFilterRegularExpression(regularExpression);
    setTextColor(filterPatternLineEdit, textColor(style()->standardPalette()));
    }
else
    {
    filterPatternLineEdit->setToolTip(regularExpression.errorString());
    proxyModel->setFilterRegularExpression(QRegularExpression());
    setTextColor(filterPatternLineEdit, Qt::red);
    }
filterMatches();
}

void Window::filterColumnChanged()
{
proxyModel->setFilterKeyColumn(filterColumnComboBox->currentIndex());
}

void Window::sortChanged()
{
bool isChecked=sortCaseSensitivityCheckBox->isChecked();
proxyModel->setSortCaseSensitivity(isChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

int kludge_ct;      // Only exists so we know how many rows there are in total, even if a filter is in force
int Window::filterMatches()
{
int ct=proxyModel->rowCount();
char str[64];
if (ct!=0 && ct!=kludge_ct)
    strfmt(str,"%d of %d movies match filter",ct,kludge_ct);
else strfmt(str,"%d movies",kludge_ct);
setWindowTitle(str);
return(ct);
}

void Window::set_focus(int32_t imno)    // auto-focus on this movie (put at top of 'recent' list))
{
RECENT recent;
recent.put(imno);
}

int32_t Window::get_current_em_key(OMZ *e)
{
int32_t imno=proxyModel->data(proxyView->currentIndex().siblingAtColumn(COL_IMDB)).toInt();
if (e!=NULL)
    {
    strncpy(e->title,
        ((QSortFilterProxyModel*)proxyModel)->data(proxyView->currentIndex().siblingAtColumn(COL_TITLE)).toString().toStdString().c_str(),59);
    e->year=((QSortFilterProxyModel*)proxyModel)->data(proxyView->currentIndex().siblingAtColumn(COL_YEAR)).toInt();
    e->k.imno=imno;
    }                                           // UNLESS usherette program resets focus to another added/rated movie
set_focus(imno);
return(imno);
}

void Window::webLink()
{
int32_t imno=get_current_em_key(NULL);
if (imno!=0) visit_imdb_webpage(imno);
}

static char* movie_title(char *s, OMZ *e)
{return(strfmt(s,"%s (%04d)", e->title, e->year));}

void Window::whereIs()
{
OMZ e;
if (get_current_em_key(&e)==0) return;
char str[1024];
QMessageBox mb;
mb.setWindowTitle(movie_title(str,&e));
QFont qf(this->font().family().toStdString().c_str(), this->font().pointSize()+2, QFont::Bold);
mb.setFont(qf);
    {   // go down a level so mvdb gets closed before mb.exec()
    MVDB mvdb;
    BL_CARGO blc;
    blc.number=0;
    *str=0;
    while (mvdb.get(BK_GT, &blc, NULL))
        {
        DYNTBL tbl_imsz(sizeof(IMSZ), cp_int32_t);
        if (!mvdb.get(BK_EQ, &blc, &tbl_imsz)) m_finish("impossible!!");
        IMSZ im;
        im.imno=e.k.imno;
        int i=tbl_imsz.in(&im);
        if (i!=NOTFND)
            {
            IMSZ *_im=(IMSZ*)(tbl_imsz.get(i));
            strendfmt(str,"\nDisk %02d %s",blc.number,str_size64(_im->sz));
            }
        }
    if (*str==0) strcpy(str,"No backup copy of movie!");
    }
mb.setText(str);
mb.exec();
}

#define HI_Y "<span foreground='green' font_weight='ultrabold'><big><big>"
#define HI_N "</big></big></span>"
#define WIDTH "--width=800 --formatted "

#define SIB(n) ((QSortFilterProxyModel*)proxyModel)->data(proxyView->currentIndex().siblingAtColumn(n))
#define SIBi(n) (SIB(n).toInt())
#define SIBs(n) (SIB(n).toString().toStdString().c_str())

void Window::rating1(void)  // can only get here if user_not_Steve
{
char cmd[1024], buf[1024], p[8];
int32_t imno=SIBi(COL_IMDB);
strncpy(buf,SIBs(COL_TITLE),59);

int i=stridxc('&',buf); if (i!=NOTFND) strins(&buf[i+1],"amp;");

strendfmt(buf," (%4d)",SIB(COL_YEAR).toInt());
strncpy(p,SIBs(COL_RATING1),sizeof(p)-1);
if (p[1]=='.') strdel(&p[1],1);
int prv=a2i(p,0);
strfmt(cmd,"yad --scale --text=\"%s   %s%s\"", HI_Y, buf, HI_N);
strendfmt(cmd," --title=\"Movie Rating\" %s ", WIDTH);
strendfmt(cmd,"%s\"%d\" ", "--value=", (prv==NOTFND)?0:prv);  // Default 0.0 if no previous rating file
strendfmt(cmd,"%s", "--min-value=\"0\" --max-value=\"99\" –step=\"1\"");
strendfmt(cmd," %s", "--mark=1:10 --mark=2:20 --mark=3:30 --mark=4:40 --mark=5:50 --mark=6:60 --mark=7:70 --mark=8:80 --mark=9:90");
//sjhlog("cmd:\n%s\n",cmd);
DYNAG *t=exec2tbl(cmd);
if (t==NULL) return;
if (t->ct!=1) {sjhlog("Unexpected error setting MyRating"); throw 79;}
int rating = a2i((char*)t->get(0),2);
//sjhlog("ret:\n%s\n",(char*)t->get(0));
delete t;
if (rating!=prv && rating!=0)
    {
    OMDB1 omdb1(false);                 // We're updating *.usr, not the MASTER database *.mst
    omdb1.put_rating(imno, rating, 0);
    }
set_focus(imno);
}

void Window::usrTxt()
{
OMZ e;
if (get_current_em_key(&e) == 0) return;
USRTXT ut(e.k.imno);  // Class defined in omdb.h, accesses OMDB object internally
std::string txt = ut.get();
QDialog dialog(this);
char ttl[100];
dialog.setWindowTitle(movie_title(ttl, &e));
QTextEdit *qd = new QTextEdit(&dialog);
qd->setText(QString::fromStdString(txt));
QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
QVBoxLayout *layout = new QVBoxLayout;
layout->addWidget(qd);
layout->addWidget(buttonBox);
dialog.setLayout(layout);

QSettings Qs;
QVariant posValue = Qs.value("usrTxtPos");
QVariant sizeValue = Qs.value("usrTxtSize");
if (posValue.isValid()) dialog.move(posValue.toPoint());
if (sizeValue.isValid()) dialog.resize(sizeValue.toSize());
if (dialog.exec() != QDialog::Accepted) return;
ut.put(qd->toPlainText().toStdString().c_str());
Qs.setValue("usrTxtPos", dialog.pos());
Qs.setValue("usrTxtSize", dialog.size());
}


void Window::ReStart(bool toggle)
{
if (toggle)
   {
   PARM parm;
   const char *p1=stradup(parm.get("$1")), *p2=stradup(parm.get("$2"));
   parm.set("$1",p2); parm.set("$2",p1);
   memtake(p1); memtake(p2);
   }
QString app = QCoreApplication::applicationFilePath();      // Get the application's executable path
QStringList args = QCoreApplication::arguments();
args.removeFirst();                     // Remove the first argument which is the application's path
QProcess::startDetached(app, args);     // Start a new instance of the application
QCoreApplication::exit();               // Exit the current application
}

void Window::contextMenuEvent(QContextMenuEvent *event)
{
QMenu menu(this);
menu.addAction(whereIsAct);
menu.addAction(webLinkAct);
if (user_not_steve)
   menu.addAction(rating1Act);
menu.addAction(usrTxtAct);
menu.addAction(ReStartAct);
menu.addAction(ReStartXAct);
QPoint qp=event->globalPos();
menu.exec(qp);
}
