#ifndef Live_h_
#define Live_h_

#include "ui_Live.h"
#include "VideoRender.h"
#include "WndList.h"
#include <opencv2/highgui/highgui.hpp>  
#include <opencv2/imgproc/imgproc.hpp>  
#include <opencv2/core/core.hpp>  

#define MaxShowMembers 50

enum E_RoomUserType
{
	E_RoomUserInvalid = -1,
	E_RoomUserWatcher, //����
	E_RoomUserCreator, //����
	E_RoomUserJoiner,  //������
};

struct RoomMember
{
	QString szID;
	E_RoomUserType userType;
};

class Live : public QDialog
{
	Q_OBJECT
public:
	Live(QWidget * parent = 0, Qt::WindowFlags f = 0);

	void setRoomID(int roomID);
	void setRoomUserType(E_RoomUserType userType);
	void ChangeRoomUserType();

	void dealMessage(const Message& message);
	void parseCusMessage(const std::string& sender, std::string msg);
	void dealCusMessage(const std::string& sender, int nUserAction, QString szActionParam);

	void StartTimer();
	void stopTimer();
	void onMixStream(std::string streamCode);

	static void OnMemStatusChange(E_EndpointEventId event_id, const Vector<String> &ids, void* data);
	static void OnRoomDisconnect(int reason, const char *errorinfo, void* data);
	static void OnDeviceDetect(void* data);
	static void OnLocalVideo(const LiveVideoFrame* video_frame, void* custom_data);
	static void OnRemoteVideo(const LiveVideoFrame* video_frame, void* custom_data);
	static void OnMessage(const Message& msg, void* data);
	static void OnDeviceOperation(E_DeviceOperationType oper, int retCode, void* data);
	static void OnQualityParamCallback(const iLiveRoomStatParam& param, void* data);

	private slots:
	void OnBtnOpenCamera();
	void OnBtnOpenCamera2();
	void OnBtnCloseCamera();
	void OnBtnCloseCamera2();
	void OnBtnOpenMic();
	void OnBtnCloseMic();
	void OnBtnOpenPlayer();
	void OnBtnClosePlayer();
	void OnBtnOpenScreenShareArea();
	void OnBtnOpenScreenShareWnd();
	void OnBtnUpdateScreenShare();
	void OnBtnCloseScreenShare();
	void OnBtnOpenSystemVoiceInput();
	void OnBtnCloseSystemVoiceInput();
	void OnBtnSendGroupMsg();
	void OnBtnStartRecord();
	void OnBtnStopRecord();
	void OnBtnPraise();
	void OnBtnSelectMediaFile();
	void OnBtnPlayMediaFile();
	void OnBtnStopMediaFile();
	void OnHsPlayerVol(int value);
	void OnSbPlayerVol(int value);
	void OnHsMicVol(int value);
	void OnSbMicVol(int value);
	void OnHsSystemVoiceInputVol(int value);
	void OnSbSystemVoiceInputVol(int value);
	void OnVsSkinSmoothChanged(int value);
	void OnSbSkinSmoothChanged(int value);
	void OnVsSkinWhiteChanged(int value);
	void OnSbSkinWhiteChanged(int value);
	void OnHsMediaFileRateChanged(int value);
	void OnHeartBeatTimer();
	void OnFillFrameTimer();
	void OnPlayMediaFileTimer();
	void OnMemberListMenu(QPoint point);
	void OnActInviteInteract();
	void OnActCancelInteract();
	void on_btnMix_clicked();


protected:
	void closeEvent(QCloseEvent* event) override;

private:
	//�Զ���˽�к���
	void updateCameraList();
	VideoRender* findVideoRender(std::string szIdentifier);
	VideoRender* getFreeVideoRender(std::string szIdentifier);
	void freeAllCameraVideoRender();

	void addMsgLab(QString msg);

	void updateMemberList();
	void updateMsgs();

	void updateCameraGB();
	void updatePlayerGB();

	void updateMicGB();
	void updateScreenShareGB();
	void updateSystemVoiceInputGB();
	void updateMediaFilePlayGB();
	void updateRecordGB();
	void updatePlayMediaFileProgress();
	void doStartPlayMediaFile();
	void doPausePlayMediaFile();
	void doResumePlayMediaFile();
	void doStopPlayMediaFile();

	void doAutoStopRecord();

	//�豸�����ص�
	void OnOpenCameraCB(const int& retCode);
	void OnCloseCameraCB(const int& retCode);

	void OnOpenExternalCaptureCB(const int& retCode);
	void OnCloseExternalCaptureCB(const int& retCode);

	void OnOpenMicCB(const int& retCode);
	void OnCloseMicCB(const int& retCode);

	void OnOpenPlayerCB(const int& retCode);
	void OnClosePlayerCB(const int& retCode);

	void OnOpenScreenShareCB(const int& retCode);
	void OnCloseScreenShareCB(const int& retCode);

	void OnOpenSystemVoiceInputCB(const int& retCode);
	void OnCloseSystemVoiceInputCB(const int& retCode);

	void OnOpenPlayMediaFileCB(const int& retCode);
	void OnClosePlayMediaFileCB(const int& retCode);

	//����㺯��
	void sendInviteInteract();//��������ͨ���ڷ�����������
	void sendCancelInteract();//�����������еĹ��ڷ�����������
	static void OnSendInviteInteractSuc(void* data);
	static void OnSendInviteInteractErr(const int code, const char *desc, void* data);

	void acceptInteract();//��ͨ���ڽ�����������
	void refuseInteract();//��ͨ���ھܾ���������
	void OnAcceptInteract();

	void exitInteract();//�������ִ�����������Ķ�������
	void OnExitInteract();

	void sendQuitRoom();//���������˳���������

						//���Ĳ�������������غ���
	void sxbCreatorQuitRoom();
	void sxbWatcherOrJoinerQuitRoom();
	void sxbHeartBeat();
	void sxbRoomIdList();
	void sxbReportrecord();
	static void OnSxbCreatorQuitRoom(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData);
	static void OnSxbWatcherOrJoinerQuitRoom(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData);
	static void OnSxbHeartBeat(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData);
	static void OnSxbRoomIdList(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData);
	static void OnSxbReportrecord(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData);

	//iLiveSDK��غ���
	void iLiveQuitRoom();
	void iLiveChangeRole(const std::string& szControlRole);
	int iLiveSetSkinSmoothGrade(int grade);
	int iLiveSetSkinWhitenessGrade(int grade);
	static void OnQuitRoomSuc(void* data);
	static void OnQuitRoomErr(int code, const char *desc, void* data);
	static void OnChangeRoleSuc(void* data);
	static void OnChangeRoleErr(int code, const char *desc, void* data);

	static void OnSendGroupMsgSuc(void* data);
	static void OnSendGroupMsgErr(int code, const char *desc, void* data);

	static void OnStartRecordVideoSuc(void* data);
	static void OnStartRecordVideoErr(int code, const char *desc, void* data);

	static void OnStopRecordSuc(Vector<String>& value, void* data);
	static void OnStopRecordVideoErr(int code, const char *desc, void* data);

	static void OnStartPushStreamSuc(PushStreamRsp &value, void *data);
	static void OnStartPushStreamErr(int code, const char * desc, void* data);

	static void OnStopPushStreamSuc(void* data);
	static void OnStopPushStreamErr(int code, const char *desc, void* data);

private:
	Ui::Live		m_ui;

	E_RoomUserType  m_userType;

	VideoRender*	m_pLocalCameraRender;
	VideoRender*	m_pScreenShareRender;

	Vector< Pair<String/*id*/, String/*name*/> > m_cameraList;

	std::vector<VideoRender*>	m_pRemoteVideoRenders;

	int					m_nRoomSize;
	QVector<RoomMember> m_roomMemberList;

	QTimer*			m_pTimer;
	QTimer*			m_pFillFrameTimer;
	QTimer*			m_pPlayMediaFileTimer;

	int				m_nCurSelectedMember;
	QMenu*			m_pMenuInviteInteract;
	QMenu*			m_pMenuCancelInteract;

	QString				m_inputRecordName;
	RecordOption		m_recordOpt;
	PushStreamOption	m_pushOpt;
	uint64				m_channelId;
	std::list<LiveUrl>	m_pushUrls;

	int32	m_x0;
	int32	m_y0;
	int32	m_x1;
	int32	m_y1;
	uint32	m_fps;

	int64	m_n64Pos;
	int64	m_n64MaxPos;

	bool	m_bRoomDisconnectClose;

	bool	m_bRecording;
	bool	m_bPushing;

	QString	m_szMsgs;
	int mRoomId;


	cv::VideoCapture* m_assistVideo;

	bool m_assVideoSetWin = false;

	const LPCWSTR m_assistName = L"AssistVideo";

	HWND m_assistHandle;
};

#endif//Live_h_