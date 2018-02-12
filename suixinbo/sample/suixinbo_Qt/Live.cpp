#include "stdafx.h"
#include "Live.h"
#include "json/json.h"
#include "MixStreamHelper.h"


//���·�תRGB֡
static bool reverseRGBFrame(LiveVideoFrame& frame)
{
	if (frame.desc.colorFormat != COLOR_FORMAT_RGB24)
	{
		return false;
	}

	LPBYTE backupBuf = (LPBYTE)malloc(frame.dataSize);
	if (!backupBuf)
	{
		return false;
	}
	memcpy(backupBuf, frame.data, frame.dataSize);

	const UINT nWidth = frame.desc.width;
	const UINT nHeight = frame.desc.height;
	for (UINT i = 0; i < frame.desc.height; ++i)
	{
		memcpy(frame.data + i * nWidth * 3, backupBuf + (nHeight - 1 - i) * nWidth * 3, nWidth * 3);
	}
	free(backupBuf);

	return true;
}

Live::Live(QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0*/)
	:QDialog(parent, f)
{
	m_ui.setupUi(this);

	m_userType = E_RoomUserInvalid;

	m_pLocalCameraRender = m_ui.widLocalVideo;
	m_pScreenShareRender = m_ui.widScreenShare;
	m_pRemoteVideoRenders.push_back(m_ui.widRemoteVideo0);
	m_pRemoteVideoRenders.push_back(m_ui.widRemoteVideo1);
	m_pRemoteVideoRenders.push_back(m_ui.widRemoteVideo2);

	m_x0 = 0;
	m_y0 = 0;
	m_fps = 10;
	QDesktopWidget* desktopWidget = QApplication::desktop();
	QRect screenRect = desktopWidget->screenGeometry();
	m_x1 = screenRect.width();
	m_y1 = screenRect.height();
	m_ui.sbX0->setValue(m_x0);
	m_ui.sbY0->setValue(m_y0);
	m_ui.sbX1->setValue(m_x1);
	m_ui.sbY1->setValue(m_y1);

	m_n64Pos = 0;
	m_n64MaxPos = 0;
	updatePlayMediaFileProgress();

	m_nRoomSize = 0;

	m_pTimer = new QTimer(this);
	m_pFillFrameTimer = new QTimer(this);
	m_pPlayMediaFileTimer = new QTimer(this);

	m_nCurSelectedMember = -1;

	m_pMenuInviteInteract = new QMenu(this);
	QAction* pActInviteInteract = new QAction(QString::fromLocal8Bit("����"), m_pMenuInviteInteract);
	m_pMenuInviteInteract->addAction(pActInviteInteract);
	m_pMenuCancelInteract = new QMenu(this);
	QAction* pActCancelInteract = new QAction(QString::fromLocal8Bit("�Ͽ�"), m_pMenuInviteInteract);
	m_pMenuCancelInteract->addAction(pActCancelInteract);

	m_channelId = 0;

	m_bRoomDisconnectClose = false;

	m_bRecording = false;
	m_bPushing = false;

	m_assistVideo = NULL;

	connect(m_ui.btnOpenCamera, SIGNAL(clicked()), this, SLOT(OnBtnOpenCamera()));
	connect(m_ui.btnOpenCamera_2, SIGNAL(clicked()), this, SLOT(OnBtnOpenCamera2()));
	connect(m_ui.btnCloseCamera, SIGNAL(clicked()), this, SLOT(OnBtnCloseCamera()));
	connect(m_ui.btnCloseCamera_2, SIGNAL(clicked()), this, SLOT(OnBtnCloseCamera2()));
	connect(m_ui.btnOpenMic, SIGNAL(clicked()), this, SLOT(OnBtnOpenMic()));
	connect(m_ui.btnCloseMic, SIGNAL(clicked()), this, SLOT(OnBtnCloseMic()));
	connect(m_ui.btnOpenPlayer, SIGNAL(clicked()), this, SLOT(OnBtnOpenPlayer()));
	connect(m_ui.btnClosePlayer, SIGNAL(clicked()), this, SLOT(OnBtnClosePlayer()));
	connect(m_ui.btnOpenScreenShareArea, SIGNAL(clicked()), this, SLOT(OnBtnOpenScreenShareArea()));
	connect(m_ui.btnOpenScreenShareWnd, SIGNAL(clicked()), this, SLOT(OnBtnOpenScreenShareWnd()));
	connect(m_ui.btnUpdateScreenShare, SIGNAL(clicked()), this, SLOT(OnBtnUpdateScreenShare()));
	connect(m_ui.btnCloseScreenShare, SIGNAL(clicked()), this, SLOT(OnBtnCloseScreenShare()));
	connect(m_ui.btnOpenSystemVoiceInput, SIGNAL(clicked()), this, SLOT(OnBtnOpenSystemVoiceInput()));
	connect(m_ui.btnCloseSystemVoiceInput, SIGNAL(clicked()), this, SLOT(OnBtnCloseSystemVoiceInput()));
	connect(m_ui.btnSendGroupMsg, SIGNAL(clicked()), this, SLOT(OnBtnSendGroupMsg()));
	connect(m_ui.btnStartRecord, SIGNAL(clicked()), this, SLOT(OnBtnStartRecord()));
	connect(m_ui.btnStopRecord, SIGNAL(clicked()), this, SLOT(OnBtnStopRecord()));
	connect(m_ui.btnSelectMediaFile, SIGNAL(clicked()), this, SLOT(OnBtnSelectMediaFile()));
	connect(m_ui.btnPlayMediaFile, SIGNAL(clicked()), this, SLOT(OnBtnPlayMediaFile()));
	connect(m_ui.btnStopMediaFile, SIGNAL(clicked()), this, SLOT(OnBtnStopMediaFile()));
	connect(m_ui.hsPlayerVol, SIGNAL(valueChanged(int)), this, SLOT(OnHsPlayerVol(int)));
	connect(m_ui.sbPlayerVol, SIGNAL(valueChanged(int)), this, SLOT(OnSbPlayerVol(int)));
	connect(m_ui.hsMicVol, SIGNAL(valueChanged(int)), this, SLOT(OnHsMicVol(int)));
	connect(m_ui.sbMicVol, SIGNAL(valueChanged(int)), this, SLOT(OnSbMicVol(int)));
	connect(m_ui.hsSystemVoiceInputVol, SIGNAL(valueChanged(int)), this, SLOT(OnHsSystemVoiceInputVol(int)));
	connect(m_ui.sbSystemVoiceInputVol, SIGNAL(valueChanged(int)), this, SLOT(OnSbSystemVoiceInputVol(int)));
	connect(m_ui.vsSkinSmooth, SIGNAL(valueChanged(int)), this, SLOT(OnVsSkinSmoothChanged(int)));
	connect(m_ui.sbSkinSmooth, SIGNAL(valueChanged(int)), this, SLOT(OnSbSkinSmoothChanged(int)));
	connect(m_ui.vsSkinWhite, SIGNAL(valueChanged(int)), this, SLOT(OnVsSkinWhiteChanged(int)));
	connect(m_ui.sbSkinWhite, SIGNAL(valueChanged(int)), this, SLOT(OnSbSkinWhiteChanged(int)));
	connect(m_ui.hsMediaFileRate, SIGNAL(valueChanged(int)), this, SLOT(OnHsMediaFileRateChanged(int)));
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnHeartBeatTimer()));
	connect(m_pFillFrameTimer, SIGNAL(timeout()), this, SLOT(OnFillFrameTimer()));
	connect(m_pPlayMediaFileTimer, SIGNAL(timeout()), this, SLOT(OnPlayMediaFileTimer()));
	connect(pActInviteInteract, SIGNAL(triggered()), this, SLOT(OnActInviteInteract()));
	connect(pActCancelInteract, SIGNAL(triggered()), this, SLOT(OnActCancelInteract()));
}

void Live::setRoomID(int roomID)
{
	m_ui.sbRoomID->setValue(roomID);
	mRoomId = roomID;
}

void Live::setRoomUserType(E_RoomUserType userType)
{
	m_userType = userType;
	m_ui.SkinGB->setVisible(false);
	updateMsgs();
	updateCameraGB();
	updatePlayerGB();

	updateMicGB();
	updateScreenShareGB();
	updateSystemVoiceInputGB();
	updateMediaFilePlayGB();
	updateRecordGB();
	switch (m_userType)
	{
	case E_RoomUserCreator:
	{
		this->setWindowTitle(QString::fromLocal8Bit("����"));
		m_ui.cameraGB->setVisible(true);
		m_ui.microphoneGB->setVisible(true);
		m_ui.screenShareGB->setVisible(true);
		m_ui.SystemVoiceInputGB->setVisible(true);
		m_ui.MediaFileGB->setVisible(true);
		m_ui.recordGB->setVisible(true);
		break;
	}
	case E_RoomUserJoiner:
	{
		this->setWindowTitle(QString::fromLocal8Bit("������"));
		m_ui.cameraGB->setVisible(true);

		m_ui.microphoneGB->setVisible(true);
		m_ui.screenShareGB->setVisible(true);
		m_ui.SystemVoiceInputGB->setVisible(true);
		m_ui.MediaFileGB->setVisible(true);
		m_ui.recordGB->setVisible(false);
		break;
	}
	case E_RoomUserWatcher:
	{
		this->setWindowTitle(QString::fromLocal8Bit("����"));
		m_ui.cameraGB->setVisible(false);

		m_ui.microphoneGB->setVisible(false);
		m_ui.screenShareGB->setVisible(false);
		m_ui.SystemVoiceInputGB->setVisible(false);
		m_ui.MediaFileGB->setVisible(false);
		m_ui.recordGB->setVisible(false);

		break;
	}
	}
}

void Live::ChangeRoomUserType()
{
	if (m_userType == E_RoomUserWatcher)
	{
		OnAcceptInteract();
	}
	else if (m_userType == E_RoomUserJoiner)
	{
		OnExitInteract();
	}
}

void Live::dealMessage(const Message& message)
{
	std::string szSender = message.sender.c_str();
	for (int i = 0; i < message.elems.size(); ++i)
	{
		MessageElem *pElem = message.elems[i];
		switch (pElem->type)
		{
		case TEXT:
		{
			QString szShow = QString::fromStdString(szSender + ": ");
			MessageTextElem *elem = dynamic_cast<MessageTextElem*>(pElem);
			addMsgLab(szShow + elem->content.c_str());
			break;
		}
		case CUSTOM:
		{
			MessageCustomElem *elem = dynamic_cast<MessageCustomElem*>(pElem);
			std::string szExt = elem->ext.c_str();
			//if (szExt==LiveNoti) //��ǰ�汾�ݲ����ô�������,������ƽ̨һ������
			{
				std::string szDate = elem->data.c_str();
				parseCusMessage(szSender, szDate);
			}
			break;
		}
		default:
			break;
		}
	}
}

void Live::parseCusMessage(const std::string& sender, std::string msg)
{
	QString qmsg = QString::fromStdString(msg);
	QJsonDocument doc = QJsonDocument::fromJson(qmsg.toLocal8Bit());
	if (doc.isObject())
	{
		QJsonObject obj = doc.object();
		QVariantMap varmap = obj.toVariantMap();
		int nUserAction = AVIMCMD_None;
		QString szActionParam;
		if (varmap.contains("userAction"))
		{
			nUserAction = varmap.value("userAction").toInt();
		}
		if (varmap.contains("actionParam"))
		{
			szActionParam = varmap.value("actionParam").toString();
		}
		dealCusMessage(sender, nUserAction, szActionParam);
	}
}

void Live::dealCusMessage(const std::string& sender, int nUserAction, QString szActionParam)
{
	switch (nUserAction)
	{
	case AVIMCMD_Multi_Host_Invite: //�����յ���������
	{
		if (sender != g_pMainWindow->getCurRoomInfo().szId.toStdString())//�����·�����������������
		{
			break;
		}
		QMessageBox::StandardButton ret = QMessageBox::question(this, FromBits("����"), FromBits("���������������Ƿ����?"));
		if (ret == QMessageBox::Yes)
		{
			acceptInteract();
		}
		else
		{
			refuseInteract();
		}
		break;
	}
	case AVIMCMD_Multi_CancelInteract: //�����յ���������
	{
		if (m_userType == E_RoomUserJoiner && szActionParam == g_pMainWindow->getUserId()) //�����������������
		{
			exitInteract();
		}
		break;
	}
	case AVIMCMD_Multi_Interact_Join: //�����յ����ڽ�����������Ļظ�
	{
		sxbRoomIdList(); //���������б�
		break;
	}
	case AVIMCMD_Multi_Interact_Refuse://�����յ����ھܾ���������Ļظ�
	{
		ShowTips(FromBits("��ʾ"), szActionParam + FromBits("�ܾ��������������."), this);
		break;
	}
	case AVIMCMD_EnterLive:
	{
		int i;
		for (i = 0; i < m_roomMemberList.size(); ++i)
		{
			if (m_roomMemberList[i].szID == szActionParam)
			{
				break;
			}
		}
		if (i == m_roomMemberList.size())
		{
			RoomMember roomMember;
			roomMember.szID = szActionParam;
			roomMember.userType = E_RoomUserWatcher;
			m_roomMemberList.push_back(roomMember);
			m_nRoomSize++;
			updateMemberList();
		}
		addMsgLab(QString::fromStdString(sender) + FromBits("���뷿��"));
		break;
	}
	case AVIMCMD_ExitLive:
	{
		if (m_userType != E_RoomUserCreator)
		{
			close();
			ShowTips(FromBits("�����˳�����"), FromBits("�����Ѿ��˳�����."), g_pMainWindow);
		}
		break;
	}
	default:
		break;
	}
}

void Live::StartTimer()
{
	sxbRoomIdList();
	m_pTimer->start(10000); //���Ĳ���̨Ҫ��10���ϱ�һ������
}

void Live::stopTimer()
{
	m_pTimer->stop();
}

void Live::OnMemStatusChange(E_EndpointEventId event_id, const Vector<String> &ids, void* data)
{
	Live* pLive = reinterpret_cast<Live*>(data);
	switch (event_id)
	{
	case EVENT_ID_ENDPOINT_HAS_CAMERA_VIDEO:
	{
		for (int i = 0; i < ids.size(); ++i)
		{
			VideoRender* pRender = (g_pMainWindow->getUserId() == ids[i].c_str()) ? pLive->m_pLocalCameraRender : pLive->getFreeVideoRender(ids[i].c_str());
			if (pRender) pRender->setView(ids[i], VIDEO_SRC_TYPE_CAMERA);
		}
		break;
	}
	case EVENT_ID_ENDPOINT_NO_CAMERA_VIDEO:
	{
		for (int i = 0; i < ids.size(); ++i)
		{
			VideoRender* pRender = (g_pMainWindow->getUserId() == ids[i].c_str()) ? pLive->m_pLocalCameraRender : pLive->findVideoRender(ids[i].c_str());
			if (pRender) pRender->remove();
		}
		break;
	}
	case EVENT_ID_ENDPOINT_HAS_SCREEN_VIDEO:
	{
		pLive->m_pScreenShareRender->setView(ids[0], VIDEO_SRC_TYPE_SCREEN);
		break;
	}
	case EVENT_ID_ENDPOINT_NO_SCREEN_VIDEO:
	{
		pLive->m_pScreenShareRender->remove();
		break;
	}
	case EVENT_ID_ENDPOINT_HAS_MEDIA_VIDEO:
	{
		pLive->m_pScreenShareRender->setView(ids[0], VIDEO_SRC_TYPE_MEDIA);
		break;
	}
	case EVENT_ID_ENDPOINT_NO_MEDIA_VIDEO:
	{
		pLive->m_pScreenShareRender->remove();
		break;
	}
	default:
		break;
	}
}

void Live::OnRoomDisconnect(int reason, const char *errorinfo, void* data)
{
	Live* pThis = reinterpret_cast<Live*>(data);
	pThis->m_bRoomDisconnectClose = true;
	pThis->close();
	ShowCodeErrorTips(reason, errorinfo, pThis, FromBits("�ѱ�ǿ���˳�����."));
}

void Live::OnDeviceDetect(void* data)
{
	Live* pThis = reinterpret_cast<Live*>(data);
	pThis->updateCameraGB();
	pThis->updateMicGB();
	pThis->updatePlayerGB();
}

void Live::OnLocalVideo(const LiveVideoFrame* video_frame, void* custom_data)
{
	Live* pLive = reinterpret_cast<Live*>(custom_data);

	if (video_frame->desc.srcType == VIDEO_SRC_TYPE_SCREEN || video_frame->desc.srcType == VIDEO_SRC_TYPE_MEDIA)
	{
		pLive->m_pScreenShareRender->doRender(video_frame);
	}
	else if (video_frame->desc.srcType == VIDEO_SRC_TYPE_CAMERA)
	{
		pLive->m_pLocalCameraRender->doRender(video_frame);
	}
}

void Live::OnRemoteVideo(const LiveVideoFrame* video_frame, void* custom_data)
{
	Live* pLive = reinterpret_cast<Live*>(custom_data);
	if (video_frame->desc.srcType == VIDEO_SRC_TYPE_SCREEN || video_frame->desc.srcType == VIDEO_SRC_TYPE_MEDIA)
	{
		pLive->m_pScreenShareRender->doRender(video_frame);
	}
	else if (video_frame->desc.srcType == VIDEO_SRC_TYPE_CAMERA)
	{
		VideoRender* pRender = pLive->findVideoRender(video_frame->identifier.c_str());
		if (pRender)
		{
			pRender->doRender(video_frame);
		}
		else
		{
			//iLiveLog_e("suixinbo", "Render is not enough.");
		}
	}
}

void Live::OnMessage(const Message& msg, void* data)
{
	Live* pThis = reinterpret_cast<Live*>(data);
	if (pThis->isVisible())
	{
		pThis->dealMessage(msg);
	}
}

void Live::OnDeviceOperation(E_DeviceOperationType oper, int retCode, void* data)
{
	Live* pThis = reinterpret_cast<Live*>(data);
	switch (oper)
	{
	case E_OpenCamera:
	{
		pThis->OnOpenCameraCB(retCode);
		break;
	}
	case E_CloseCamera:
	{
		pThis->OnCloseCameraCB(retCode);
		break;
	}
	case E_OpenExternalCapture:
	{
		pThis->OnOpenExternalCaptureCB(retCode);
		break;
	}
	case E_CloseExternalCapture:
	{
		pThis->OnCloseExternalCaptureCB(retCode);
		break;
	}
	case E_OpenMic:
	{
		pThis->OnOpenMicCB(retCode);
		break;
	}
	case E_CloseMic:
	{
		pThis->OnCloseMicCB(retCode);
		break;
	}
	case E_OpenPlayer:
	{
		pThis->OnOpenPlayerCB(retCode);
		break;
	}
	case E_ClosePlayer:
	{
		pThis->OnClosePlayerCB(retCode);
		break;
	}
	case E_OpenScreenShare:
	{
		pThis->OnOpenScreenShareCB(retCode);
		break;
	}
	case E_CloseScreenShare:
	{
		pThis->OnCloseScreenShareCB(retCode);
		break;
	}
	case E_OpenSystemVoiceInput:
	{
		pThis->OnOpenSystemVoiceInputCB(retCode);
		break;
	}
	case E_CloseSystemVoiceInput:
	{
		pThis->OnCloseSystemVoiceInputCB(retCode);
		break;
	}
	case E_OpenPlayMediaFile:
	{
		pThis->OnOpenPlayMediaFileCB(retCode);
		break;
	}
	case E_ClosePlayMediaFile:
	{
		pThis->OnClosePlayMediaFileCB(retCode);
		break;
	}
	}
}

void Live::OnQualityParamCallback(const iLiveRoomStatParam& param, void* data)
{
	printf("%s\n", param.getInfoString().c_str());
}

void Live::OnBtnOpenCamera()
{
	if (m_cameraList.size() == 0)
	{
		ShowErrorTips(FromBits("�޿��õ�����ͷ."), this);
		return;
	}
	m_ui.btnOpenCamera->setEnabled(false);
	int ndx = m_ui.cbCamera->currentIndex();
	GetILive()->openCamera(m_cameraList[ndx].first.c_str());
}
void Live::OnBtnOpenCamera2()
{
	if (m_cameraList.size() == 0)
	{
		ShowErrorTips(FromBits("�޿��õ�����ͷ."), this);
		return;
	}
	m_ui.btnOpenCamera_2->setEnabled(false);
	m_ui.btnCloseCamera_2->setEnabled(true);
	int ndx = m_ui.cbCamera_2->currentIndex();


	if (m_assistVideo != NULL)
	{
		delete m_assistVideo;
		m_assistVideo = NULL;
	}

	m_assistVideo = new cv::VideoCapture(ndx);
	if (!m_assistVideo->isOpened())
	{
		ShowErrorTips(FromBits("��������ͷ����ʧ��."), this);
		return;
	}


	m_pFillFrameTimer->start(66); // ֡��1000/66Լ����15
}

void Live::OnBtnCloseCamera()
{
	m_ui.btnCloseCamera->setEnabled(false);
	GetILive()->closeCamera();
}

void Live::OnBtnCloseCamera2()
{
	m_ui.btnCloseCamera_2->setEnabled(false);
	m_ui.btnOpenCamera_2->setEnabled(true);
}

void Live::OnBtnOpenMic()
{
	m_ui.btnOpenMic->setEnabled(false);

	Vector< Pair<String/*id*/, String/*name*/> > micList;
	int nRet = GetILive()->getMicList(micList);
	if (nRet != NO_ERR)
	{
		m_ui.btnOpenMic->setEnabled(true);
		ShowCodeErrorTips(nRet, "get Mic List Failed.", this);
		return;
	}
	GetILive()->openMic(micList[0].first);
}

void Live::OnBtnCloseMic()
{
	m_ui.btnCloseMic->setEnabled(false);
	GetILive()->closeMic();
}

void Live::OnBtnOpenPlayer()
{
	m_ui.btnOpenPlayer->setEnabled(false);
	Vector< Pair<String, String> > playerList;
	int nRet = GetILive()->getPlayerList(playerList);
	if (nRet != NO_ERR)
	{
		m_ui.btnOpenPlayer->setEnabled(true);
		ShowCodeErrorTips(nRet, "Get Player List Failed.", this);
		return;
	}
	GetILive()->openPlayer(playerList[0].first);
}

void Live::OnBtnClosePlayer()
{
	m_ui.btnClosePlayer->setEnabled(false);
	GetILive()->closePlayer();
}

void Live::OnBtnOpenScreenShareArea()
{
	m_ui.btnOpenScreenShareArea->setEnabled(false);
	m_x0 = m_ui.sbX0->value();
	m_y0 = m_ui.sbY0->value();
	m_x1 = m_ui.sbX1->value();
	m_y1 = m_ui.sbY1->value();
	GetILive()->openScreenShare(m_x0, m_y0, m_x1, m_y1, m_fps);
}

void Live::OnBtnOpenScreenShareWnd()
{
	m_ui.btnOpenScreenShareWnd->setEnabled(false);

	HWND hwnd = WndList::GetSelectWnd();
	if (!hwnd)
	{
		m_ui.btnOpenScreenShareWnd->setEnabled(true);
		return;
	}
	GetILive()->openScreenShare(hwnd, m_fps);
}

void Live::OnBtnUpdateScreenShare()
{
	m_ui.btnUpdateScreenShare->setEnabled(false);

	m_x0 = m_ui.sbX0->value();
	m_y0 = m_ui.sbY0->value();
	m_x1 = m_ui.sbX1->value();
	m_y1 = m_ui.sbY1->value();

	int nRet = GetILive()->changeScreenShareSize(m_x0, m_y0, m_x1, m_y1);
	if (nRet != NO_ERR)
	{
		m_ui.btnUpdateScreenShare->setEnabled(true);
		ShowCodeErrorTips(nRet, "changeScreenShareAreaSize failed.", this);
		return;
	}
	updateScreenShareGB();
}

void Live::OnBtnCloseScreenShare()
{
	m_ui.btnCloseScreenShare->setEnabled(false);
	GetILive()->closeScreenShare();
}

void Live::OnBtnOpenSystemVoiceInput()
{
	m_ui.btnOpenSystemVoiceInput->setEnabled(false);
	//GetILive()->openSystemVoiceInput("D:\\Program Files (x86)\\KuGou\\KGMusic\\KuGou.exe"); //���ɼ�ָ��������������
	GetILive()->openSystemVoiceInput();//�ɼ�ϵͳ�е���������
}

void Live::OnBtnCloseSystemVoiceInput()
{
	m_ui.btnCloseSystemVoiceInput->setEnabled(false);
	GetILive()->closeSystemVoiceInput();
}

void Live::OnBtnSendGroupMsg()
{
	QString szText = m_ui.teEditText->toPlainText();
	m_ui.teEditText->setPlainText("");
	if (szText.isEmpty())
	{
		return;
	}

	Message msg;
	MessageTextElem *elem = new MessageTextElem(String(szText.toStdString().c_str()));
	msg.elems.push_back(elem);
	addMsgLab(QString::fromLocal8Bit("��˵��") + szText);
	GetILive()->sendGroupMessage(msg, OnSendGroupMsgSuc, OnSendGroupMsgErr, this);
}

void Live::OnBtnStartRecord()
{
	bool bClickedOK;
	m_inputRecordName = QInputDialog::getText(this, FromBits("¼���ļ���"), FromBits("������¼���ļ���"), QLineEdit::Normal, "", &bClickedOK);
	if (!bClickedOK)//�û������ȡ����ť
	{
		return;
	}
	if (m_inputRecordName.isEmpty())
	{
		ShowErrorTips(FromBits("¼���ļ�������Ϊ��"), this);
		return;
	}
	//���Ĳ���̨Ҫ��¼���ļ���ͳһ��ʽ"sxb_�û�id_�û�������ļ���"
	QString fileName = "sxb_";
	fileName += g_pMainWindow->getUserId();
	fileName += "_";
	fileName += m_inputRecordName;

	m_recordOpt.fileName = fileName.toStdString().c_str();
	m_recordOpt.recordDataType = (E_RecordDataType)m_ui.cbRecordDataType->itemData(m_ui.cbRecordDataType->currentIndex()).value<int>();
	GetILive()->startRecord(m_recordOpt, OnStartRecordVideoSuc, OnStartRecordVideoErr, this);
}

void Live::OnBtnStopRecord()
{
	E_RecordDataType recordDataType = (E_RecordDataType)m_ui.cbRecordDataType->itemData(m_ui.cbRecordDataType->currentIndex()).value<int>();
	GetILive()->stopRecord(recordDataType, OnStopRecordSuc, OnStopRecordVideoErr, this);
}



void Live::OnBtnPraise()
{
	g_sendGroupCustomCmd(AVIMCMD_Praise, g_pMainWindow->getUserId());
	addMsgLab(g_pMainWindow->getUserId() + FromBits("����"));
}

void Live::OnBtnSelectMediaFile()
{
	QString szMediaPath = QFileDialog::getOpenFileName(this, FromBits("��ѡ�񲥷ŵ���Ƶ�ļ�"), "", FromBits("��Ƶ�ļ�(*.aac *.ac3 *.amr *.ape *.mp3 *.flac *.midi *.wav *.wma *.ogg *.amv *.mkv *.mod *.mts *.ogm *.f4v *.flv *.hlv *.asf *.avi *.wm *.wmp *.wmv *.ram *.rm *.rmvb *.rpm *.rt *.smi *.dat *.m1v *.m2p *.m2t *.m2ts *.m2v *.mp2v *.tp *.tpr *.ts *.m4b *.m4p *.m4v *.mp4 *.mpeg4 *.3g2 *.3gp *.3gp2 *.3gpp *.mov *.pva *.dat *.m1v *.m2p *.m2t *.m2ts *.m2v *.mp2v *.pss *.pva *.ifo *.vob *.divx *.evo *.ivm *.mkv *.mod *.mts *.ogm *.scm *.tod *.vp6 *.webm *.xlmv)"));
	if (!szMediaPath.isEmpty())
	{
		m_ui.edMediaFilePath->setText(szMediaPath);
	}
}

void Live::OnBtnPlayMediaFile()
{
	E_PlayMediaFileState state = GetILive()->getPlayMediaFileState();
	if (state == E_PlayMediaFileStop)//ֹͣ->����
	{
		doStartPlayMediaFile();
	}
	else if (state == E_PlayMediaFilePlaying) //����->��ͣ
	{
		doPausePlayMediaFile();
	}
	else if (state == E_PlayMediaFilePause)//��ͣ->�ָ�����
	{
		doResumePlayMediaFile();
	}
}

void Live::OnBtnStopMediaFile()
{
	E_PlayMediaFileState state = GetILive()->getPlayMediaFileState();
	if (state == E_PlayMediaFileStop)
	{
		return;
	}
	doStopPlayMediaFile();
}

void Live::OnHsPlayerVol(int value)
{
	m_ui.sbPlayerVol->blockSignals(true);
	m_ui.sbPlayerVol->setValue(value);
	m_ui.sbPlayerVol->blockSignals(false);

	GetILive()->setPlayerVolume(value);
}

void Live::OnSbPlayerVol(int value)
{
	m_ui.hsPlayerVol->blockSignals(true);
	m_ui.hsPlayerVol->setValue(value);
	m_ui.hsPlayerVol->blockSignals(false);

	GetILive()->setPlayerVolume(value);
}

void Live::OnHsMicVol(int value)
{
	m_ui.sbMicVol->blockSignals(true);
	m_ui.sbMicVol->setValue(value);
	m_ui.sbMicVol->blockSignals(false);

	GetILive()->setMicVolume(value);
}

void Live::OnSbMicVol(int value)
{
	m_ui.hsMicVol->blockSignals(true);
	m_ui.hsMicVol->setValue(value);
	m_ui.hsMicVol->blockSignals(false);

	GetILive()->setMicVolume(value);
}

void Live::OnHsSystemVoiceInputVol(int value)
{
	m_ui.sbSystemVoiceInputVol->blockSignals(true);
	m_ui.sbSystemVoiceInputVol->setValue(value);
	m_ui.sbSystemVoiceInputVol->blockSignals(false);

	GetILive()->setSystemVoiceInputVolume(value);
}

void Live::OnSbSystemVoiceInputVol(int value)
{
	m_ui.hsSystemVoiceInputVol->blockSignals(true);
	m_ui.hsSystemVoiceInputVol->setValue(value);
	m_ui.hsSystemVoiceInputVol->blockSignals(false);

	GetILive()->setSystemVoiceInputVolume(value);
}

void Live::OnVsSkinSmoothChanged(int value)
{
	m_ui.sbSkinSmooth->blockSignals(true);
	m_ui.sbSkinSmooth->setValue(value);
	m_ui.sbSkinSmooth->blockSignals(false);
	iLiveSetSkinSmoothGrade(value);
}

void Live::OnSbSkinSmoothChanged(int value)
{
	m_ui.vsSkinSmooth->blockSignals(true);
	m_ui.vsSkinSmooth->setValue(value);
	m_ui.vsSkinSmooth->blockSignals(false);
	iLiveSetSkinSmoothGrade(value);
}

void Live::OnVsSkinWhiteChanged(int value)
{
	m_ui.sbSkinWhite->blockSignals(true);
	m_ui.sbSkinWhite->setValue(value);
	m_ui.sbSkinWhite->blockSignals(false);
	iLiveSetSkinWhitenessGrade(value);
}

void Live::OnSbSkinWhiteChanged(int value)
{
	m_ui.vsSkinWhite->blockSignals(true);
	m_ui.vsSkinWhite->setValue(value);
	m_ui.vsSkinWhite->blockSignals(false);
	iLiveSetSkinWhitenessGrade(value);
}

void Live::OnHsMediaFileRateChanged(int value)
{
	int nRet = GetILive()->setPlayMediaFilePos(value);
	if (nRet != NO_ERR)
	{
		ShowCodeErrorTips(nRet, FromBits("���ò��Ž���ʧ��"), this);
	}
}

void Live::OnHeartBeatTimer()
{
	sxbHeartBeat();
	sxbRoomIdList();
}

void Live::OnFillFrameTimer()
{
// 	cv::Mat tmpFrame;
// 	(*m_assistVideo) >> tmpFrame;
// 
// 	cv::imshow("��ǰ��Ƶ", tmpFrame);
// 	
// 	LiveVideoFrame tmpLiveframe;
// 	tmpLiveframe.data = (uint8*)tmpFrame.data;
// 	tmpLiveframe.dataSize = tmpFrame.cols * tmpFrame.rows * 3;
// 	tmpLiveframe.desc.colorFormat = COLOR_FORMAT_RGB24;
// 	tmpLiveframe.desc.srcType = VIDEO_SRC_TYPE_MEDIA;
// 	tmpLiveframe.desc.width = tmpFrame.cols;
// 	tmpLiveframe.desc.height = tmpFrame.rows;
// 	tmpLiveframe.desc.rotate = 0;
// 
// 	reverseRGBFrame(tmpLiveframe);
// 
// 	int nRet = GetILive()->fillExternalCaptureFrame(tmpLiveframe);

//////////////////////////////////////////////
//������ʾ�Զ���ɼ�����ȡһ�ű���ͼƬ��ÿһ֡�������ͼƬ��Ϊ��������
	HBITMAP hbitmap = (HBITMAP)LoadImageA(NULL, "ExternalCapture.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (!hbitmap)
	{
		return;
	}

	BITMAP bitmap;
	GetObject(hbitmap, sizeof(BITMAP), &bitmap);

	/////////////////������ֵ�ͼƬ��start////////////////
	HDC hDC = GetDC((HWND)(this->winId()));
	HDC hMemDC = CreateCompatibleDC(hDC);
	SelectObject(hMemDC, hbitmap);

	char chFont[20];
	HFONT hfont = CreateFontA(100, 0, 0, 0, 400, 0, 0, 0, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, chFont);

	SelectObject(hMemDC, hfont);
	TextOutA(hMemDC, 0, 0, "�����Զ���ɼ��Ļ���", strlen("�����Զ���ɼ��Ļ���"));

	DeleteObject(hfont);
	DeleteObject(hMemDC);
	ReleaseDC((HWND)(this->winId()), hDC);
	/////////////////������ֵ�ͼƬ��end////////////////

	LiveVideoFrame frame;
	frame.data = (uint8*)bitmap.bmBits;
	frame.dataSize = bitmap.bmWidth * bitmap.bmHeight * 3;
	frame.desc.colorFormat = COLOR_FORMAT_RGB24;
	frame.desc.width = bitmap.bmWidth;
	frame.desc.height = bitmap.bmHeight;
	frame.desc.rotate = 0;
	reverseRGBFrame(frame);//����BMPͼƬ�����������µߵ�����ģ�������Ҫ���·�ת��

	int nRet = GetILive()->fillExternalCaptureFrame(frame);
	if (nRet != NO_ERR)
	{
		m_pFillFrameTimer->stop();
		if (nRet != NO_ERR)
		{
			ShowCodeErrorTips(nRet, FromBits("�Զ���ɼ���Ƶ�������"), this);
		}
	}

	DeleteObject(hbitmap);
}

void Live::OnPlayMediaFileTimer()
{
	int nRet = GetILive()->getPlayMediaFilePos(m_n64Pos, m_n64MaxPos);
	if (nRet == NO_ERR)
	{
		if (m_n64Pos >= m_n64MaxPos)
		{
			int nRet = GetILive()->restartMediaFile();
			if (nRet != NO_ERR)
			{
				m_pPlayMediaFileTimer->stop();
				ShowErrorTips(FromBits("�Զ��ز�ʧ��"), this);
			}
		}
		updatePlayMediaFileProgress();
	}
	else
	{
		//Just do nothing.
	}
}

void Live::closeEvent(QCloseEvent* event)
{
	if (!m_bRoomDisconnectClose)
	{
		if (m_userType == E_RoomUserCreator)//�����˳�������Ҫ�����Ĳ��������ϱ��˳�����
		{
			sendQuitRoom();
			sxbCreatorQuitRoom();
		}
		else//���ں��������ֻ��Ҫ�����Ĳ��������ϱ��Լ���ID
		{
			sxbWatcherOrJoinerQuitRoom();
		}
	}
	else
	{
		g_pMainWindow->setUseable(true);
	}
	m_bRecording = false;
	m_bPushing = false;
	m_bRoomDisconnectClose = false;
	m_szMsgs = "";
	freeAllCameraVideoRender();
	stopTimer();
	if (m_pFillFrameTimer->isActive())
	{
		m_pFillFrameTimer->stop();
	}
	if (m_pPlayMediaFileTimer->isActive())
	{
		m_pPlayMediaFileTimer->stop();
	}

	event->accept();
}

void Live::updateCameraList()
{
	GetILive()->getCameraList(m_cameraList);
	m_ui.cbCamera->clear();
	m_ui.cbCamera_2->clear();

	for (int i = 0; i < m_cameraList.size(); ++i)
	{
		m_ui.cbCamera->addItem(FromBits(m_cameraList[i].second.c_str()));
		m_ui.cbCamera_2->addItem(FromBits(m_cameraList[i].second.c_str()));
	}
	m_ui.cbCamera->setCurrentIndex(0);
	m_ui.cbCamera_2->setCurrentIndex(0);
}

VideoRender* Live::findVideoRender(std::string szIdentifier)
{
	for (unsigned i = 0; i < m_pRemoteVideoRenders.size(); ++i)
	{
		if (szIdentifier == m_pRemoteVideoRenders[i]->getIdentifier().c_str())
		{
			return m_pRemoteVideoRenders[i];
		}
	}
	return NULL;
}

VideoRender* Live::getFreeVideoRender(std::string szIdentifier)
{
	for (unsigned i = 0; i < m_pRemoteVideoRenders.size(); ++i)
	{
		if (m_pRemoteVideoRenders[i]->isFree())
		{
			m_pRemoteVideoRenders[i]->setView(szIdentifier.c_str(), VIDEO_SRC_TYPE_CAMERA);
			return m_pRemoteVideoRenders[i];
		}
	}
	return NULL;
}

void Live::freeAllCameraVideoRender()
{
	m_pLocalCameraRender->remove();
	m_pScreenShareRender->remove();
	for (size_t i = 0; i < m_pRemoteVideoRenders.size(); ++i)
	{
		m_pRemoteVideoRenders[i]->remove();
	}
}

void Live::addMsgLab(QString msg)
{
	const int nMaxLen = 2000;
	m_szMsgs += msg;
	m_szMsgs += "\n";
	if (m_szMsgs.length() > nMaxLen)
	{
		m_szMsgs = m_szMsgs.right(nMaxLen);
	}
	updateMsgs();
}

void Live::updateMemberList()
{
	m_ui.sbTotalMemNum->setValue(m_nRoomSize);
	m_ui.liMembers->clear();
	for (int i = 0; i < m_roomMemberList.size(); ++i)
	{
		RoomMember& member = m_roomMemberList[i];
		QString szShowName = member.szID;
		switch (member.userType)
		{
		case E_RoomUserJoiner:
			szShowName += QString::fromLocal8Bit("(����)");
			break;
		case E_RoomUserCreator:
			szShowName += QString::fromLocal8Bit("(����)");
			break;
		case E_RoomUserWatcher:
			break;
		}
		m_ui.liMembers->addItem(new QListWidgetItem(szShowName));
	}
}

void Live::updateMsgs()
{
	m_ui.teMsgs->setPlainText(m_szMsgs);
	QTextCursor cursor = m_ui.teMsgs->textCursor();
	cursor.movePosition(QTextCursor::End);
	m_ui.teMsgs->setTextCursor(cursor);
}

void Live::updateCameraGB()
{
	if (GetILive()->getExternalCaptureState())
	{
		m_ui.cameraGB->setEnabled(false);
	}
	else
	{
		m_ui.cameraGB->setEnabled(true);
	}

	if (GetILive()->getCurCameraState())
	{
		m_ui.btnOpenCamera->setEnabled(false);
		m_ui.btnCloseCamera->setEnabled(true);
	}
	else
	{
		m_ui.btnOpenCamera->setEnabled(true);
		m_ui.btnCloseCamera->setEnabled(false);
		updateCameraList();
	}
}

void Live::updatePlayerGB()
{
	m_ui.sbPlayerVol->blockSignals(true);
	m_ui.hsPlayerVol->blockSignals(true);

	if (GetILive()->getCurPlayerState())
	{
		m_ui.sbPlayerVol->setEnabled(true);
		m_ui.hsPlayerVol->setEnabled(true);
		uint32 uVol = GetILive()->getPlayerVolume();
		m_ui.sbPlayerVol->setValue(uVol);
		m_ui.hsPlayerVol->setValue(uVol);
		m_ui.btnOpenPlayer->setEnabled(false);
		m_ui.btnClosePlayer->setEnabled(true);
	}
	else
	{
		m_ui.sbPlayerVol->setValue(0);
		m_ui.hsPlayerVol->setValue(0);
		m_ui.sbPlayerVol->setEnabled(false);
		m_ui.hsPlayerVol->setEnabled(false);
		m_ui.btnOpenPlayer->setEnabled(true);
		m_ui.btnClosePlayer->setEnabled(false);
	}

	m_ui.sbPlayerVol->blockSignals(false);
	m_ui.hsPlayerVol->blockSignals(false);
}

void Live::updateMicGB()
{
	m_ui.sbMicVol->blockSignals(true);
	m_ui.hsMicVol->blockSignals(true);

	if (GetILive()->getCurMicState())
	{
		m_ui.sbMicVol->setEnabled(true);
		m_ui.hsMicVol->setEnabled(true);
		uint32 uVol = GetILive()->getMicVolume();
		m_ui.sbMicVol->setValue(uVol);
		m_ui.hsMicVol->setValue(uVol);
		m_ui.btnOpenMic->setEnabled(false);
		m_ui.btnCloseMic->setEnabled(true);
	}
	else
	{
		m_ui.sbMicVol->setValue(0);
		m_ui.hsMicVol->setValue(0);
		m_ui.sbMicVol->setEnabled(false);
		m_ui.hsMicVol->setEnabled(false);
		m_ui.btnOpenMic->setEnabled(true);
		m_ui.btnCloseMic->setEnabled(false);
	}

	m_ui.sbMicVol->blockSignals(false);
	m_ui.hsMicVol->blockSignals(false);
}

void Live::updateScreenShareGB()
{
	if (GetILive()->getPlayMediaFileState() != E_PlayMediaFileStop)
	{
		m_ui.screenShareGB->setEnabled(false);
	}
	else
	{
		m_ui.screenShareGB->setEnabled(true);
	}

	m_ui.sbX0->blockSignals(true);
	m_ui.sbY0->blockSignals(true);
	m_ui.sbX1->blockSignals(true);
	m_ui.sbY1->blockSignals(true);
	m_ui.sbX0->setValue(m_x0);
	m_ui.sbY0->setValue(m_y0);
	m_ui.sbX1->setValue(m_x1);
	m_ui.sbY1->setValue(m_y1);
	m_ui.sbX0->blockSignals(false);
	m_ui.sbY0->blockSignals(false);
	m_ui.sbX1->blockSignals(false);
	m_ui.sbY1->blockSignals(false);

	E_ScreenShareState state = GetILive()->getScreenShareState();
	switch (state)
	{
	case E_ScreenShareNone:
	{
		m_ui.sbX0->setEnabled(true);
		m_ui.sbY0->setEnabled(true);
		m_ui.sbX1->setEnabled(true);
		m_ui.sbY1->setEnabled(true);
		m_ui.btnOpenScreenShareArea->setEnabled(true);
		m_ui.btnUpdateScreenShare->setEnabled(false);
		m_ui.btnOpenScreenShareWnd->setEnabled(true);
		m_ui.btnCloseScreenShare->setEnabled(false);
		break;
	}
	case E_ScreenShareArea:
	{
		m_ui.sbX0->setEnabled(true);
		m_ui.sbY0->setEnabled(true);
		m_ui.sbX1->setEnabled(true);
		m_ui.sbY1->setEnabled(true);
		m_ui.btnOpenScreenShareArea->setEnabled(false);
		m_ui.btnUpdateScreenShare->setEnabled(true);
		m_ui.btnOpenScreenShareWnd->setEnabled(false);
		m_ui.btnCloseScreenShare->setEnabled(true);
		break;
	}
	case E_ScreenShareWnd:
	{
		m_ui.sbX0->setEnabled(false);
		m_ui.sbY0->setEnabled(false);
		m_ui.sbX1->setEnabled(false);
		m_ui.sbY1->setEnabled(false);
		m_ui.btnOpenScreenShareArea->setEnabled(false);
		m_ui.btnUpdateScreenShare->setEnabled(false);
		m_ui.btnOpenScreenShareWnd->setEnabled(false);
		m_ui.btnCloseScreenShare->setEnabled(true);
		break;
	}
	}
}

void Live::updateSystemVoiceInputGB()
{
	m_ui.sbSystemVoiceInputVol->blockSignals(true);
	m_ui.hsSystemVoiceInputVol->blockSignals(true);

	if (GetILive()->getCurSystemVoiceInputState())
	{
		m_ui.sbSystemVoiceInputVol->setEnabled(true);
		m_ui.hsSystemVoiceInputVol->setEnabled(true);
		uint32 uVol = GetILive()->getSystemVoiceInputVolume();
		m_ui.sbSystemVoiceInputVol->setValue(uVol);
		m_ui.hsSystemVoiceInputVol->setValue(uVol);
		m_ui.btnOpenSystemVoiceInput->setEnabled(false);
		m_ui.btnCloseSystemVoiceInput->setEnabled(true);
	}
	else
	{
		m_ui.sbSystemVoiceInputVol->setValue(0);
		m_ui.hsSystemVoiceInputVol->setValue(0);
		m_ui.sbSystemVoiceInputVol->setEnabled(false);
		m_ui.hsSystemVoiceInputVol->setEnabled(false);
		m_ui.btnOpenSystemVoiceInput->setEnabled(true);
		m_ui.btnCloseSystemVoiceInput->setEnabled(false);
	}

	m_ui.sbSystemVoiceInputVol->blockSignals(false);
	m_ui.hsSystemVoiceInputVol->blockSignals(false);
}

void Live::updateMediaFilePlayGB()
{
	if (GetILive()->getScreenShareState() != E_ScreenShareNone)
	{
		m_ui.MediaFileGB->setEnabled(false);
	}
	else
	{
		m_ui.MediaFileGB->setEnabled(true);
	}

	E_PlayMediaFileState state = GetILive()->getPlayMediaFileState();
	switch (state)
	{
	case E_PlayMediaFileStop:
	{
		m_ui.btnSelectMediaFile->setEnabled(true);
		m_ui.btnPlayMediaFile->setText(FromBits("����"));
		m_ui.btnPlayMediaFile->setEnabled(true);
		m_ui.btnStopMediaFile->setEnabled(false);
		m_n64MaxPos = 0;
		m_n64Pos = 0;
		updatePlayMediaFileProgress();
		break;
	}
	case E_PlayMediaFilePlaying:
	{
		m_ui.btnSelectMediaFile->setEnabled(false);
		m_ui.btnPlayMediaFile->setText(FromBits("��ͣ"));
		m_ui.btnPlayMediaFile->setEnabled(true);
		m_ui.btnStopMediaFile->setEnabled(true);
		break;
	}
	case E_PlayMediaFilePause:
	{
		m_ui.btnSelectMediaFile->setEnabled(false);
		m_ui.btnPlayMediaFile->setText(FromBits("����"));
		m_ui.btnPlayMediaFile->setEnabled(true);
		m_ui.btnStopMediaFile->setEnabled(true);
		break;
	}
	}
}

void Live::updateRecordGB()
{
	int nRecordDataTypeIndex = m_ui.cbRecordDataType->currentIndex();
	m_ui.cbRecordDataType->clear();

	if ((!GetILive()->getCurCameraState()) && (!GetILive()->getExternalCaptureState())
		&& (!GetILive()->getScreenShareState()) && (!GetILive()->getPlayMediaFileState()))
	{
		m_ui.recordGB->setEnabled(false);
		return;
	}

	m_ui.recordGB->setEnabled(true);
	if (m_bRecording)
	{
		m_ui.btnStartRecord->setEnabled(false);
		m_ui.btnStopRecord->setEnabled(true);
		m_ui.cbRecordDataType->setEnabled(false);
	}
	else
	{
		m_ui.btnStartRecord->setEnabled(true);
		m_ui.btnStopRecord->setEnabled(false);
		m_ui.cbRecordDataType->setEnabled(true);
	}

	if (GetILive()->getCurCameraState() || GetILive()->getExternalCaptureState())
	{
		m_ui.cbRecordDataType->addItem(FromBits("��·(����ͷ/�Զ���ɼ�)"), QVariant(E_RecordCamera));
	}
	if (GetILive()->getScreenShareState() || GetILive()->getPlayMediaFileState())
	{
		m_ui.cbRecordDataType->addItem(FromBits("��·(��Ļ����/�ļ�����)"), QVariant(E_RecordScreen));
	}
	nRecordDataTypeIndex = iliveMin(m_ui.cbRecordDataType->count() - 1, iliveMax(0, nRecordDataTypeIndex));
	m_ui.cbRecordDataType->setCurrentIndex(nRecordDataTypeIndex);
}


void Live::updatePlayMediaFileProgress()
{
	m_ui.hsMediaFileRate->blockSignals(true);
	m_ui.lbMediaFileRate->setText(getTimeStrBySeconds(m_n64Pos) + "/" + getTimeStrBySeconds(m_n64MaxPos));
	m_ui.hsMediaFileRate->setMinimum(0);
	m_ui.hsMediaFileRate->setMaximum(m_n64MaxPos);
	m_ui.hsMediaFileRate->setValue(m_n64Pos);
	int nStep = m_n64MaxPos / 40;
	nStep = nStep < 1 ? 1 : nStep;
	m_ui.hsMediaFileRate->setSingleStep(nStep);
	m_ui.hsMediaFileRate->setPageStep(nStep);
	m_ui.hsMediaFileRate->blockSignals(false);
}

void Live::doStartPlayMediaFile()
{
	QString szMediaPath = m_ui.edMediaFilePath->text();

	if (szMediaPath.isEmpty())
	{
		ShowErrorTips(FromBits("����ѡ��Ҫ���ŵ��ļ�"), this);
		return;
	}

	QFile file(szMediaPath);
	if (!file.exists())
	{
		ShowErrorTips(FromBits("ָ�����ļ������ڣ�������ѡ��"), this);
		return;
	}

	if (!GetILive()->isValidMediaFile(szMediaPath.toStdString().c_str()))
	{
		ShowErrorTips(FromBits("��֧�ֲ�����ѡ�ļ�,������ѡ��."), this);
		return;
	}

	GetILive()->openPlayMediaFile(szMediaPath.toStdString().c_str());
}

void Live::doPausePlayMediaFile()
{
	int nRet = GetILive()->pausePlayMediaFile();
	if (nRet == NO_ERR)
	{
		m_pPlayMediaFileTimer->stop();
		m_ui.btnPlayMediaFile->setText(FromBits("����"));
	}
	else
	{
		ShowCodeErrorTips(nRet, FromBits("��ͣ���ų���"), this);
	}
}

void Live::doResumePlayMediaFile()
{
	int nRet = GetILive()->resumePlayMediaFile();
	if (nRet == NO_ERR)
	{
		m_pPlayMediaFileTimer->start(1000);
		m_ui.btnPlayMediaFile->setText(FromBits("��ͣ"));
	}
	else
	{
		ShowCodeErrorTips(nRet, FromBits("�ָ����ų���"), this);
	}
}

void Live::doStopPlayMediaFile()
{
	GetILive()->closePlayMediaFile();
}

void Live::doAutoStopRecord()
{
	if (m_bRecording)
	{
		int nRecordDataTypeIndex = m_ui.cbRecordDataType->currentIndex();
		E_RecordDataType eSelectedType = (E_RecordDataType)m_ui.cbRecordDataType->itemData(nRecordDataTypeIndex).value<int>();
		switch (eSelectedType)
		{
		case E_RecordCamera:
		{
			if ((!GetILive()->getCurCameraState()) && (!GetILive()->getExternalCaptureState()))
			{
				OnBtnStopRecord();
			}
			break;
		}
		case E_RecordScreen:
		{
			if ((!GetILive()->getScreenShareState()) && (!GetILive()->getPlayMediaFileState()))
			{
				OnBtnStopRecord();
			}
			break;
		}
		}
	}
	else
	{
		updateRecordGB();
	}
}


void Live::OnOpenCameraCB(const int& retCode)
{
	updateCameraGB();

	updateRecordGB();

	if (retCode == NO_ERR)
	{
		m_ui.SkinGB->setVisible(true);
	}
	else
	{
		ShowCodeErrorTips(retCode, "Open Camera Failed.", this);
	}
}

void Live::OnCloseCameraCB(const int& retCode)
{
	updateCameraGB();

	doAutoStopRecord();

	if (retCode == NO_ERR)
	{
		m_ui.SkinGB->setVisible(false);
		m_pLocalCameraRender->update();
	}
	else
	{
		ShowCodeErrorTips(retCode, "Close Camera Failed.", this);
	}
}

void Live::OnOpenExternalCaptureCB(const int& retCode)
{

	updateCameraGB();
	updateRecordGB();

	if (retCode == NO_ERR)
	{
		m_pFillFrameTimer->start(66); // ֡��1000/66Լ����15
	}
	else
	{
		ShowCodeErrorTips(retCode, FromBits("���Զ���ɼ�ʧ��"), this);
	}
}

void Live::OnCloseExternalCaptureCB(const int& retCode)
{

	updateCameraGB();
	doAutoStopRecord();

	if (retCode == NO_ERR)
	{
		m_pFillFrameTimer->stop();
		m_pLocalCameraRender->update();
	}
	else
	{
		ShowCodeErrorTips(retCode, FromBits("�ر��Զ���ɼ�ʧ��"), this);
	}
}

void Live::OnOpenMicCB(const int& retCode)
{
	updateMicGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Open Mic Failed.", this);
	}
}

void Live::OnCloseMicCB(const int& retCode)
{
	updateMicGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Close Mic Failed.", this);
	}
}

void Live::OnOpenPlayerCB(const int& retCode)
{
	updatePlayerGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Open Player Failed.", this);
	}
}

void Live::OnClosePlayerCB(const int& retCode)
{
	updatePlayerGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Close Player Failed.", this);
	}
}

void Live::OnOpenScreenShareCB(const int& retCode)
{
	updateScreenShareGB();
	updateMediaFilePlayGB();
	updateRecordGB();

	if (retCode != NO_ERR)
	{
		if (retCode == 1002)
		{
			ShowErrorTips(FromBits("��Ļ������ļ����Ų���ͬʱ��."), this);
		}
		else if (retCode == 1008)
		{
			ShowErrorTips(FromBits("������������Ա�Ѿ�ռ�ø�·��Ƶ."), this);
		}
		else
		{
			ShowCodeErrorTips(retCode, FromBits("����Ļ����ʧ��."), this);
		}
	}
}

void Live::OnCloseScreenShareCB(const int& retCode)
{
	updateScreenShareGB();
	updateMediaFilePlayGB();
	doAutoStopRecord();

	if (retCode == NO_ERR)
	{
		m_pScreenShareRender->update();
	}
	else
	{
		ShowCodeErrorTips(retCode, "Close Screen Share Failed.", this);
	}
}

void Live::OnOpenSystemVoiceInputCB(const int& retCode)
{
	updateSystemVoiceInputGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Open System Voice Input Failed.", this);
	}
}

void Live::OnCloseSystemVoiceInputCB(const int& retCode)
{
	updateSystemVoiceInputGB();
	if (retCode != NO_ERR)
	{
		ShowCodeErrorTips(retCode, "Close SystemVoiceInput Failed.", this);
	}
}

void Live::OnOpenPlayMediaFileCB(const int& retCode)
{
	updateMediaFilePlayGB();
	updateScreenShareGB();
	updateRecordGB();
	if (retCode == NO_ERR)
	{
		m_pPlayMediaFileTimer->start(1000);
	}
	else if (retCode == 1002)
	{
		ShowErrorTips(FromBits("��Ļ������ļ����Ų���ͬʱ��"), this);
	}
	else if (retCode == 1008)
	{
		ShowErrorTips(FromBits("������������Ա�Ѿ�ռ�ø�·��Ƶ"), this);
	}
	else
	{
		ShowCodeErrorTips(retCode, FromBits("���ļ����ų���."), this);
	}
}

void Live::OnClosePlayMediaFileCB(const int& retCode)
{
	updateMediaFilePlayGB();
	updateScreenShareGB();
	doAutoStopRecord();

	if (retCode == NO_ERR)
	{
		m_pPlayMediaFileTimer->stop();
		m_pScreenShareRender->update();
	}
	else
	{
		ShowCodeErrorTips(retCode, FromBits("�ر��ļ����ų���"), this);
	}
}

void Live::sendInviteInteract()
{
	g_sendC2CCustomCmd(m_roomMemberList[m_nCurSelectedMember].szID, AVIMCMD_Multi_Host_Invite, "", OnSendInviteInteractSuc, OnSendInviteInteractErr, this);
}

void Live::sendCancelInteract()
{
	g_sendGroupCustomCmd(AVIMCMD_Multi_CancelInteract, m_roomMemberList[m_nCurSelectedMember].szID);
}

void Live::OnSendInviteInteractSuc(void* data)
{
	//ShowTips( FromBits("����ɹ�"), FromBits("�ɹ��������룬�ȴ����ڽ�������."), reinterpret_cast<Live*>(data) );
}

void Live::OnSendInviteInteractErr(const int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Send Invite Interact Error.");
}

void Live::acceptInteract()
{
	iLiveChangeRole(LiveGuest);
}

void Live::refuseInteract()
{
	//֪ͨ�����ܾ�����
	g_sendC2CCustomCmd(g_pMainWindow->getCurRoomInfo().szId, AVIMCMD_Multi_Interact_Refuse, g_pMainWindow->getUserId());
}

void Live::OnAcceptInteract()
{
	setRoomUserType(E_RoomUserJoiner);
	OnBtnOpenCamera();
	OnBtnOpenMic();

	//��ݱ�Ϊ�����û������������ϱ��Լ���ݱ仯,��������ȡ�����Ա�б�
	sxbHeartBeat();
	sxbRoomIdList();
	//���������֪ͨ����
	g_sendC2CCustomCmd(g_pMainWindow->getCurRoomInfo().szId, AVIMCMD_Multi_Interact_Join, g_pMainWindow->getUserId());
}

void Live::exitInteract()
{
	iLiveChangeRole(Guest);
}

void Live::OnExitInteract()
{
	if (m_ui.btnCloseCamera->isEnabled())
	{
		OnBtnCloseCamera();
	}
	if (m_ui.btnCloseMic->isEnabled())
	{
		OnBtnCloseMic();
	}
	if (m_ui.btnCloseScreenShare->isEnabled())
	{
		OnBtnCloseScreenShare();
	}
	if (m_ui.btnCloseSystemVoiceInput->isEnabled())
	{
		OnBtnCloseSystemVoiceInput();
	}
	if (m_ui.btnStopMediaFile)
	{
		OnBtnStopMediaFile();
	}
	setRoomUserType(E_RoomUserWatcher);

	//��ݱ仯�����������ϱ���ݣ����������󷿼��Ա�б�
	sxbHeartBeat();
	sxbRoomIdList();
}

void Live::sendQuitRoom()
{
	g_sendGroupCustomCmd(AVIMCMD_ExitLive, g_pMainWindow->getUserId());
}

void Live::sxbCreatorQuitRoom()
{
	QVariantMap varmap;
	varmap.insert("token", g_pMainWindow->getToken());
	varmap.insert("roomnum", g_pMainWindow->getCurRoomInfo().info.roomnum);
	varmap.insert("type", "live");
	SxbServerHelper::request(varmap, "live", "exitroom", OnSxbCreatorQuitRoom, this);
}

void Live::sxbWatcherOrJoinerQuitRoom()
{
	QVariantMap varmap;
	varmap.insert("token", g_pMainWindow->getToken());
	varmap.insert("id", g_pMainWindow->getUserId());
	varmap.insert("roomnum", g_pMainWindow->getCurRoomInfo().info.roomnum);
	varmap.insert("role", (int)m_userType);//��Ա0 ����1 �����Ա2
	varmap.insert("operate", 1);//���뷿��0  �˳�����1
	SxbServerHelper::request(varmap, "live", "reportmemid", OnSxbWatcherOrJoinerQuitRoom, this);
}

void Live::sxbHeartBeat()
{
	QVariantMap varmap;
	varmap.insert("token", g_pMainWindow->getToken());
	varmap.insert("roomnum", g_pMainWindow->getCurRoomInfo().info.roomnum);
	varmap.insert("role", (int)m_userType); //0 ���� 1 ���� 2 �������
	if (m_userType == E_RoomUserCreator)
	{
		varmap.insert("thumbup", g_pMainWindow->getCurRoomInfo().info.thumbup);
	}
	SxbServerHelper::request(varmap, "live", "heartbeat", OnSxbHeartBeat, this);
}

void Live::sxbRoomIdList()
{
	QVariantMap varmap;
	varmap.insert("token", g_pMainWindow->getToken());
	varmap.insert("roomnum", g_pMainWindow->getCurRoomInfo().info.roomnum);
	varmap.insert("index", 0);
	varmap.insert("size", MaxShowMembers);
	SxbServerHelper::request(varmap, "live", "roomidlist", OnSxbRoomIdList, this);
}

void Live::sxbReportrecord()
{
	QVariantMap varmap;
	varmap.insert("token", g_pMainWindow->getToken());
	varmap.insert("roomnum", g_pMainWindow->getCurRoomInfo().info.roomnum);
	varmap.insert("uid", g_pMainWindow->getCurRoomInfo().szId);//������
	varmap.insert("name", m_inputRecordName);//�û������¼����
	varmap.insert("type", 0);//Ԥ���ֶΣ�����0
	varmap.insert("cover", "");//PC���Ĳ��ݲ�֧��ֱ�������ϴ�,������ʱ�ϴ�Ϊ�գ���������
	SxbServerHelper::request(varmap, "live", "reportrecord", OnSxbReportrecord, this);
}

void Live::OnSxbCreatorQuitRoom(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData)
{
	Live* pLive = reinterpret_cast<Live*>(pCusData);

	if (errorCode == E_SxbOK)
	{
		pLive->iLiveQuitRoom();
	}
	else
	{
		ShowCodeErrorTips(errorCode, errorInfo, pLive, FromBits("�����˳�����ʧ��"));
	}
}

void Live::OnSxbWatcherOrJoinerQuitRoom(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData)
{
	Live* pLive = reinterpret_cast<Live*>(pCusData);

	if (errorCode == E_SxbOK || errorCode == 10010)//�ɹ����߷����Ѿ�������
	{
		pLive->iLiveQuitRoom();
	}
	else
	{
		ShowCodeErrorTips(errorCode, errorInfo, pLive, FromBits("�����˳�����ʧ��"));
	}
}

void Live::OnSxbHeartBeat(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData)
{
	Live* pLive = reinterpret_cast<Live*>(pCusData);

	if (errorCode != E_SxbOK)
	{
		//iLiveLog_e( "suixinbo", "Sui xin bo heartbeat failed: %d %s", errorCode, errorInfo.toStdString().c_str() );
	}
}

void Live::OnSxbRoomIdList(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData)
{
	Live* pLive = reinterpret_cast<Live*>(pCusData);

	if (errorCode != E_SxbOK)
	{
		//iLiveLog_e( "suixinbo", "Suixinbo get Room id list failed: %d %s", errorCode, errorInfo.toStdString().c_str() );
		return;
	}

	if (datamap.contains("total"))
	{
		pLive->m_nRoomSize = datamap.value("total").toInt();
	}
	if (datamap.contains("idlist"))
	{
		pLive->m_roomMemberList.clear();
		QVariantList idlist = datamap.value("idlist").toList();
		for (int i = 0; i < idlist.size(); ++i)
		{
			QVariantMap idmap = idlist[i].toMap();
			RoomMember member;
			if (idmap.contains("id"))
			{
				member.szID = idmap.value("id").toString();

			}
			if (idmap.contains("role"))
			{
				member.userType = (E_RoomUserType)idmap.value("role").toInt();
			}
			pLive->m_roomMemberList.push_back(member);
		}
		pLive->updateMemberList();
	}
}

void Live::OnSxbReportrecord(int errorCode, QString errorInfo, QVariantMap datamap, void* pCusData)
{
	Live* pLive = reinterpret_cast<Live*>(pCusData);

	if (errorCode != E_SxbOK)
	{
		//iLiveLog_e( "suixinbo", "Suixinbo report record video failed: %d %s", errorCode, errorInfo.toStdString().c_str() );
		return;
	}
}

void Live::iLiveQuitRoom()
{
	GetILive()->quitRoom(OnQuitRoomSuc, OnQuitRoomErr, this);
}

void Live::iLiveChangeRole(const std::string& szControlRole)
{
	GetILive()->changeRole(szControlRole.c_str(), OnChangeRoleSuc, OnChangeRoleErr, this);
}

int Live::iLiveSetSkinSmoothGrade(int grade)
{
	return GetILive()->setSkinSmoothGrade(grade);
}

int Live::iLiveSetSkinWhitenessGrade(int grade)
{
	return GetILive()->setSkinWhitenessGrade(grade);
}

void Live::OnQuitRoomSuc(void* data)
{
	Room roominfo = g_pMainWindow->getCurRoomInfo();
	roominfo.szId = "";
	roominfo.info.thumbup = 0;
	g_pMainWindow->setCurRoomIdfo(roominfo);
	g_pMainWindow->setUseable(true);
}

void Live::OnQuitRoomErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Quit iLive Room Error.");
}

void Live::OnChangeRoleSuc(void* data)
{
	reinterpret_cast<Live*>(data)->ChangeRoomUserType();
}

void Live::OnChangeRoleErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, FromBits(desc), reinterpret_cast<Live*>(data), "Change Role Error.");
}

void Live::OnSendGroupMsgSuc(void* data)
{

}

void Live::OnSendGroupMsgErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Send Group Message Error.");
}

void Live::OnMemberListMenu(QPoint point)
{
	if (m_userType != E_RoomUserCreator)
	{
		return;
	}
	m_nCurSelectedMember = m_ui.liMembers->currentRow();
	if (m_nCurSelectedMember >= 0 && m_nCurSelectedMember < m_roomMemberList.size())
	{
		RoomMember& roomMember = m_roomMemberList[m_nCurSelectedMember];
		if (roomMember.userType == E_RoomUserWatcher)
		{
			m_pMenuInviteInteract->exec(QCursor::pos());
		}
		else if (roomMember.userType == E_RoomUserJoiner)
		{
			m_pMenuCancelInteract->exec(QCursor::pos());
		}
	}
}

void Live::OnActInviteInteract()
{
	m_nCurSelectedMember = m_ui.liMembers->currentRow();
	if (m_nCurSelectedMember >= 0 || m_nCurSelectedMember < m_ui.liMembers->count())
	{
		sendInviteInteract();
	}
}

void Live::OnActCancelInteract()
{
	m_nCurSelectedMember = m_ui.liMembers->currentRow();
	if (m_nCurSelectedMember >= 0 || m_nCurSelectedMember < m_ui.liMembers->count())
	{
		RoomMember& roomber = m_roomMemberList[m_nCurSelectedMember];
		roomber.userType = E_RoomUserWatcher;
		updateMemberList();

		sendCancelInteract();
	}
}

void Live::OnStartRecordVideoSuc(void* data)
{
	Live* pThis = reinterpret_cast<Live*>(data);
	pThis->m_bRecording = true;
	pThis->updateRecordGB();
}

void Live::OnStartRecordVideoErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Start Record Video Error.");
}

void Live::OnStopRecordSuc(Vector<String>& value, void* data)
{
	Live* pLive = reinterpret_cast<Live*>(data);
	pLive->m_bRecording = false;
	pLive->updateRecordGB();
	pLive->sxbReportrecord();
}

void Live::OnStopRecordVideoErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Stop Record Video Error.");
}

void Live::OnStartPushStreamSuc(PushStreamRsp &value, void *data)
{
	Live* pLive = reinterpret_cast<Live*>(data);
	pLive->m_bPushing = true;
	pLive->m_channelId = value.channelId;
	for (auto i = value.urls.begin(); i != value.urls.end(); ++i)
	{
		pLive->m_pushUrls.push_back(*i);
	}

}

void Live::OnStartPushStreamErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Start Push Stream Error.");
}

void Live::OnStopPushStreamSuc(void* data)
{
	Live* pLive = reinterpret_cast<Live*>(data);
	pLive->m_bPushing = false;
	pLive->m_channelId = 0;
	pLive->m_pushUrls.clear();
}

void Live::OnStopPushStreamErr(int code, const char *desc, void* data)
{
	ShowCodeErrorTips(code, desc, reinterpret_cast<Live*>(data), "Stop Push Stream Error.");
}


void Live::on_btnMix_clicked()
{
	std::vector<std::pair<std::string, bool>> list;

	//aux
	if ((m_pScreenShareRender != nullptr) && (!m_pScreenShareRender->getIdentifier().empty()))
	{
		std::pair<std::string, bool> id(m_pScreenShareRender->getIdentifier().c_str(), true);
		list.push_back(id);
	}
	//local camera
	if ((m_pLocalCameraRender != nullptr) && (!m_pLocalCameraRender->getIdentifier().empty()))
	{
		std::pair<std::string, bool> id(m_pLocalCameraRender->getIdentifier().c_str(), false);
		list.push_back(id);
	}

	//remote camera
	for (sizet i = 0; i < m_pRemoteVideoRenders.size(); ++i)
	{
		if (m_pRemoteVideoRenders[i]->isFree()) continue;
		std::pair<std::string, bool> id(m_pRemoteVideoRenders[i]->getIdentifier().c_str(), false);
		list.push_back(id);
	}

	if (list.size() < 2)
	{
		ShowErrorTips(FromBits("��������·���ſ��Խ��л���!"), this);
	}
	else
	{
		MixStreamHelper helper(list, QString("45eeb9fc2e4e6f88b778e0bbd9de3737"), mRoomId, this);
		helper.doRequest();
	}
}

void Live::onMixStream(std::string streamCode)
{
	std::stringstream ss;
	ss << "ffplay rtmp://8525.liveplay.myqcloud.com/live/" << streamCode;
	std::string url = ss.str();

	system(url.c_str());
}

