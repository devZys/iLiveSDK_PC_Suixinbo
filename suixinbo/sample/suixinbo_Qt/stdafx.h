#ifndef stdafx_h_
#define stdafx_h_

#include <QtWidgets>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

#include <set>
#include <time.h>
#include <Windows.h>

#include "iLive.h"
using namespace ilive;
#pragma comment(lib, "iLiveSDK.lib")

#include "Util.h"
#include "PicDownHelper.h"
#include "SxbServerHelper.h"
#include "MainWindow.h"

#ifdef _DEBUG
	#pragma comment(lib, "Qt5Networkd.lib")
#else
	#pragma comment(lib, "Qt5Network.lib")
#endif//_DEBUG

#define SuixinboServerUrl	"http://live.trueskom.com:8080/wawa/index.php"
#define SuixinboAppid		1400068315
#define SuixinboAccountType 22580



#define FromStdStr(str) QString::fromStdString(str)
#define FromBits(str) QString::fromLocal8Bit(str)

#define LiveMaster	"LiveMaster" //����
#define LiveGuest	"LiveGuest"  //�������
#define Guest		"Guest"		 //����

extern MainWindow* g_pMainWindow;

/////////////////////////���Ĳ�����Start///////////////////////////////
#define LiveNoti "LiveNotification" //�Զ���������

#define ILVLIVE_IMCMD_CUSTOM_LOW_LIMIT 0x800
#define ILVLIVE_IMCMD_CUSTOM_UP_LIMIT 0x900

enum E_CustomCmd
{
	AVIMCMD_None,               // ���¼���0

	AVIMCMD_EnterLive,          // �û�����ֱ��, Group��Ϣ �� 1
	AVIMCMD_ExitLive,           // �û��˳�ֱ��, Group��Ϣ �� 2
	AVIMCMD_Praise,             // ������Ϣ, Demo��ʹ��Group��Ϣ �� 3
	AVIMCMD_Host_Leave,         // �����򻥶������뿪, Group��Ϣ �� 4
	AVIMCMD_Host_Back,          // �����򻥶����ڻ���, Group��Ϣ �� 5

	AVIMCMD_Multi = ILVLIVE_IMCMD_CUSTOM_LOW_LIMIT,  // ���˻�����Ϣ���� �� 2048

	AVIMCMD_Multi_Host_Invite,          // ������������������Ϣ, C2C��Ϣ �� 2049
	AVIMCMD_Multi_CancelInteract,       // �ѽ��뻥��ʱ���Ͽ�������Group��Ϣ�����Ͽ��ߵ�imUsreid���� �� 2050
	AVIMCMD_Multi_Interact_Join,        // ���˻������յ�AVIMCMD_Multi_Host_Invite���������ͬ�⣬C2C��Ϣ �� 2051
	AVIMCMD_Multi_Interact_Refuse,      // ���˻������յ�AVIMCMD_Multi_Invite��������󣬾ܾ���C2C��Ϣ �� 2052
};
/////////////////////////���Ĳ�����End///////////////////////////////

void g_sendC2CCustomCmd( QString dstUser, E_CustomCmd userAction, QString actionParam, iLiveSucCallback suc = NULL, iLiveErrCallback err = NULL, void* data = NULL );
void g_sendGroupCustomCmd( E_CustomCmd userAction, QString actionParam, iLiveSucCallback suc = NULL, iLiveErrCallback err = NULL, void* data = NULL );

#endif//stdafx_h_