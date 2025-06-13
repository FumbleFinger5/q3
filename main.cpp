#include <QApplication>
#include <QStandardItemModel>
#include <QTime>
// This is crucial: undefine the 'signals' macro after Qt headers
// but BEFORE any GTK/GLib headers that might use 'signals' as a struct member.
#ifdef signals
#undef signals
#endif

#include <unistd.h>
#include <sys/wait.h>

#include "pdef.h"
#include "memgive.h"
#include "str.h"
#include "log.h"
#include "drinfo.h"
#include "csv.h"
#include "cal.h"
#include "omdb1.h"   // library function make_emk() returns "raw data" table of movies used to populate display
#include "exec.h"
#include "parm.h"
#include "imdb.h"
#include "scan.h"

#include "window.h"
#include "myqsettings.h"

char hide_columns[NUMCOLS];

DYNAG *global_fontname;

static void set_hide_columns(void)
{
PARM pm;
int i, j, len;
char w[128], *p;
for (i=0;i<NUMCOLS;i++) hide_columns[i]=0;
strncpy(w,pm.get("hide_columns"),127);
for (p=w;*p;p+=len)
    {
    if ((len=stridxc(COMMA,p))==NOTFND) len=strlen(p);
    char pnm[32];
    memmove(pnm,p,len); pnm[len]=0;
    for (j=NUMCOLS;j--;)        // find which column matches the name from the setting
        if (!strcmp(fld[j].fnm,pnm)) break;
//      if (!memcmp(fld[j].fnm,p,len) && (p[len]==0 || p[len]==COMMA)) break;
    if (j==NOTFND) {sjhlog("Bad 'hide columns' setting [%s]",w); throw(77);}
    hide_columns[j]=YES;
    if (p[len]) p++; // step over the comma as well as the setting name itself
    }
}

static void fail77(const char *txt)
{
sjhlog("Bad config setting [%s]",txt);
throw 77;
}

// 'terminator' may be '_' (multi-fld 'font' setting) OR '=' (overrride column heading)
static int find_fnm(const char *w, const char *p, char terminator)
{
int len, j=NUMCOLS;
while (j--)             // (but if it's a 'column rename' ONLY accept '=' after fnm)
   {
   len=strlen(fld[j].fnm);
   if (!memcmp(fld[j].fnm,p,len) && (p[len]==0 || p[len]==terminator)) break;
   }
if (j==NOTFND) fail77(w); // text after underscore wasn't a valid field-id
return(j);
}

static char *my_date(char *s, short bd)
{
if (bd) calfmt(s,"%D %3.3M'%02Y",long_bd(bd));
else s[0]=0;
return(s);
}

static char *get_notes(char *s, int32_t imno, OMDB1 *sjh, OMDB1 *mph)
{
std::string txt;
if (sjh) txt=sjh->get_notes(imno);
else if (mph) txt=mph->get_notes(imno);
//strncpy(s,txt.c_str(),59);  // Returned string is only used for main table display, so it's okay to truncate it here
strcpy(s,txt.c_str());
int i;
while ((i=stridxc(LNFEED,s))!=NOTFND)      // CRET, LNFEED
    s[i]=TAB;
return(s);
}

enum Qt::AlignmentFlag align(char ch)
{
if (ch=='L') return(Qt::AlignLeft);
if (ch=='R') return(Qt::AlignRight);
return(Qt::AlignHCenter);
}

extern int kludge_ct;

QString myfix(const char *s)
{return(QString::fromUtf8(s));}

static int32_t populateModel(Window *window, QStandardItemModel *model, UserFonts *uf)
{
RECENT recent;
model->setHeaderData(0, Qt::Horizontal, QObject::tr("Title tooltip text"),Qt::ToolTipRole);
int col;
for (col=0;col<NUMCOLS;col++)
    {
    model->setHeaderData(col, Qt::Horizontal, fld[col].name);
    model->setHeaderData(col, Qt::Horizontal, Qt::AlignHCenter , Qt::TextAlignmentRole);
    }
OMDB1   sjh(true),
        mph(false);
OM1_KEY om1,
        om2;
SCAN_ALL ia;

char wrk[8192];
const char *fn=sjh.filename();
strfmt(wrk,"QGroupBox::title { color: %s; font-weight: bold; }", running_live?"green":"red");
window->proxyGroupBox->setStyleSheet(wrk);
window->proxyGroupBox->setTitle(QString::fromUtf8(fn));

bool again=false;
kludge_ct=sjh.recct();
while (sjh.scan_all(&om2,&again))
   {
   if (!mph.get_om1(om2.imno, &om1)) memset(&om1,0,sizeof(OM1_KEY));
   model->insertRow(0);    // Column numbers model->index(row,COL) as per imdb.cpp:FLDX fld[NUMCOLS]
   model->setData(model->index(0, COL_RECENT), QVariant::fromValue<int>(recent.pos(om2.imno)), Qt::UserRole);
   const char *ttl;
   if (om2.mytitle!=0) ttl=sjh.rh2str(om2.mytitle,wrk);
   else  ttl=ia.get(om2.imno,FID_TITLE,wrk);
   QString myQString = QString::fromUtf8(ttl);
   model->setData(model->index(0, COL_TITLE), myQString);
   model->setData(model->index(0, COL_YEAR), myfix(ia.get(om2.imno,FID_YEAR,wrk)));
   model->setData(model->index(0, COL_RATING), myfix(om2.rating ? strfmt(wrk, "%1.1f", 0.1 * om2.rating) : "-"));
   model->setData(model->index(0, COL_TMDB), myfix(strfmt(wrk, "%d", om2.tmno)));
   model->setData(model->index(0, COL_IMDB), myfix(strfmt(wrk, "%d", om2.imno)));
   model->setData(model->index(0, COL_DIRECTOR), myfix(ia.get(om2.imno,FID_DIRECTOR,wrk)));
   model->setData(model->index(0, COL_ADDED), myfix(my_date(wrk,om2.added)));
   model->setData(model->index(0, COL_SEEN), (om2.seen==0) ? "-" : my_date(wrk,short_bd(om2.seen)));
   if (om2.seen!=0 && long_bd(short_bd(om2.seen))!=om2.seen)
      model->setData(model->index(0, COL_SEEN), QVariant::fromValue<int>(om2.seen), Qt::UserRole);
   model->setData(model->index(0, COL_GB), om2.sz ? strfmt(wrk,"%1.1f",0.1*om2.sz) : "-");
   model->setData(model->index(0, COL_CAST), myfix(ia.get(om2.imno,FID_CAST,wrk)));
   model->setData(model->index(0, COL_RUNTIME), myfix(ia.get(om2.imno,FID_RUNTIME,wrk)));
   model->setData(model->index(0, COL_NOTES), get_notes(wrk, om2.imno, &sjh, NULL));
   model->setData(model->index(0, COL_NOTES1), get_notes(wrk, om2.imno, NULL, &mph));
   model->setData(model->index(0, COL_RATING1), om1.rating ? strfmt(wrk, "%1.1f", 0.1 * om1.rating) : "-");
   model->setData(model->index(0, COL_GENRE), myfix(ia.get(om2.imno,FID_GENRE,wrk)));
   model->setData(model->index(0, COL_MYSEEN), om1.seen ? my_date(wrk,short_bd(om1.seen)) : "-");
   for (col=0;col<NUMCOLS;col++)
      {
      model->setData(model->index(0, col), align(fld[col].align), Qt::TextAlignmentRole);
      if (uf->fldfnt[col]!=NOTFND)
         model->setData(model->index(0, col), uf->qfcol(col), Qt::FontRole);
      }
   }
return(recent.top());
}

int mustfindcomma(const char *s)
{
int c=stridxc(COMMA,s);
if (c==NOTFND) throw 77;
return(c);
}

char *s2ss(const char *s, int n, char *ss)
{
int c;
while (c=mustfindcomma(s), n--) s+=(c+1);
if (c>0) memmove(ss,s,c);
ss[c]=0;
return(strtrim(ss));
}

void str2myfont1(char *s, MYFONT *f, DYNAG *fontname) // get font for a named QSetting
{
char ss[128];
strncpy(strend(s),",,,,",10);   // See strncat()  Stop the whinging about unsafe strcat()
f->pnam=(const char*)fontname->put(s2ss(s,0,ss));
f->ptsz=a2i(s2ss(s,1,ss),0);
f->weight=a2i(s2ss(s,2,ss),0);
f->italic=(a2i(s2ss(s,3,ss),0)!=0);
}

void str2myfont(const char *msn, MYFONT *f, DYNAG *fontname) // get font for a named QSetting
{
PARM pm;
char s[128];
strancpy(s,pm.get(msn),sizeof(s)-1);
str2myfont1(s,f,fontname);
}

UserFonts::~UserFonts()
{
delete fontname;
delete mff;
}

static void override_name(const char *w)
{
int j=find_fnm(w, &w[5], 0);
PARM pm;
char str[128];
if (!*strncpy(str,pm.get(w), 127))
    fail77(w);
if (global_fontname==NULL) global_fontname=new DYNAG(0);
fld[j].name=(const char*)global_fontname->get(global_fontname->in_or_add(str));
}

#define BASE_FONT_SETTING "font"
UserFonts::UserFonts(void)
{
fontname = new DYNAG(0);     // names of all fonts used
mff = new DYNAG(sizeof(MYFONT));
MYFONT mf;
PARM pm;
int i, j;
char w[128], *p;
for (i=0;i<NUMCOLS;i++) fldfnt[i]=NOTFND;
strncpy(w,pm.get(BASE_FONT_SETTING),127);
if (w[0])
    str2myfont1(w,&mf,fontname);   // load font setting named 'w' into 'mf'
else                        // QSetting 'font' (system default) not yet defined
    {
    QFont qf;       // First one just holds application default values
    strncpy(w,qf.family().toStdString().c_str(),sizeof(w)-1);
    mf.pnam=(const char*)fontname->get(fontname->in_or_add(w));
    mf.ptsz=qf.pointSize();
    mf.weight=(qf.weight()+5)/10;
    mf.italic=qf.italic();
    strendfmt(w,",%d,%d,%d",mf.ptsz,mf.weight,mf.italic);
    }
mff->put(&mf);      // initialise with "system default" font as element 0 in table
DYNAG *qsl=pm.allKeys();
for (i=0;i<qsl->ct; i++)
    {
    strncpy(w,(char*)qsl->get(i),127);
    if (!memcmp(w,"name_",5)) {override_name(w); continue;}
    if (memcmp(w,"font_",5)) continue;
    str2myfont(w,&mf,fontname);   // load font setting named 'w' into 'mf'
    for (p=&w[4]; p[0]; p+=strlen(fld[j].fnm)) // loop checking for multiple underscore-separated field names
        {
        if (*(p++)!='_') fail77(w);
        j=find_fnm(w, p, '_');
        if (j==NOTFND) fail77(w); // text after underscore wasn't a valid field-id
        if (memcmp(mff->get(0),&mf,sizeof(MYFONT)))     // If column j's font doesn't match default (0)
            fldfnt[j]=mff->in_or_add(&mf);              // store the font number to use for column j
        }
    }
}

QFont UserFonts::qfcol(int col)          // return the default font stored iin element 0
{
MYFONT *mf=(MYFONT*)mff->get((col==NOTFND)?0:fldfnt[col]);
QFont qf(mf->pnam,mf->ptsz,mf->weight*10,mf->italic);
return(qf);
}

static void settings_init()
{
QCoreApplication::setOrganizationName("Softworks");
QCoreApplication::setApplicationName("SMDB");
char *usr=getenv("USER");
if (usr==NULL || strcmp(usr,"steve"))   // read-only mode unless user is ME!
    user_not_steve=YES;     // Accessed by q3:window.cpp AND plib:omdb.cpp
PARM pm;
char w[128];
if (*strncpy(w,pm.get("exec"),sizeof(w)-1))   // Used to be "cache update" command for Mike. Now unused
    if (system(w)) sjhlog("exec FAILED [%s]\n",w);
running_live=(stridxs("dropbox",pm.get("$1"))!=NOTFND);  // If not dropbox-based, must be using LOCAL TEST files
}

static int app_try_catch(QApplication *app)
{
int err=NO;
try
    {
    err= app->exec();
    SCRAP(global_fontname);
    }
catch (int e)
    {
    printf("didn't work\n");
    err=e;
    }
if (err)
    printf("Failed with error %d\r\n",err);
return(err);
}

int main(int argc, char *argv[])
{
QApplication app(argc, argv);
settings_init();
Window window;

char w[256];
int sz=readlink("/proc/self/exe", w, sizeof(w));
if (sz<10) throw(88);
w[sz]=0;
strncpy(strrchr(w,'/'),"/q3.jpg",20);
if (drattrget(w,NULL))
    {
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(w)); // splash picture
    splash->show();
    QTimer::singleShot(2500, splash,SLOT(close())); // Timer
    QTimer::singleShot(2500,&window,SLOT(show()));
    }

QStandardItemModel model(0, NUMCOLS, &window);

UserFonts *uf=new UserFonts();
window.setFont(uf->qfcol(NOTFND));

int32_t recent=populateModel(&window, &model, uf);
delete uf;

window.setSourceModel(&model);

get_conf_number(argc, argv);
MyQSettings mqs(&window);

set_hide_columns();
 for (int i=NUMCOLS; i--;) window.proxyView->setColumnHidden(i, hide_columns[i]);
window.show();
window.refocus(recent);
return(app_try_catch(&app));
}
