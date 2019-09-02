#include "OGLordRobot.h"
#include "OGLordRobotAI.h"
#include "OGLordRbtAIClv.h"

using namespace std;

OGLordRobot::OGLordRobot()
{
    m_pRobot = new OGLordRobotAI();
    m_pRobotClv = new COGLordRbtAIClv();
}

OGLordRobot::~OGLordRobot()
{
    delete m_pRobot;
    m_pRobot = NULL;
    delete m_pRobotClv;
    m_pRobotClv = NULL;
}

bool OGLordRobot::RbtInSetLevel(int argLevel)
{
    if (argLevel == TRUST_LEVEL)
    {
        argLevel = BEST_LEVEL_2;
        //delete m_pRobot;
        //m_pRobot = NULL;
        //delete m_pRobotClv;
        //m_pRobotClv = new COGLordRbtAIClv();

        //return m_pRobotClv ? m_pRobotClv->RbtInSetLevel(0) : false;
    }

    if (BASE_LEVEL <= argLevel && argLevel <= BEST_LEVEL)
    {
        delete m_pRobotClv;
        m_pRobotClv = NULL;
        if (!m_pRobot)
        {
            m_pRobot = new OGLordRobotAI();
        }
        return m_pRobot ? m_pRobot->RbtInSetLevel(argLevel) : false;
    }
    else
    {
        delete m_pRobot;
        m_pRobot = NULL;
        if (!m_pRobotClv)
        {
            m_pRobotClv = new COGLordRbtAIClv();
        }
        return m_pRobotClv ? m_pRobotClv->RbtInSetLevel(argLevel - BEST_LEVEL + 2) : false; //TODO
    }
}

bool OGLordRobot::RbtInInitCard(int argSeat, /* �Լ�����λ�� */ std::vector<int> argHandCard /* ���͵����� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInInitCard(argSeat, argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInInitCard(argSeat, argHandCard) : false;
}

bool OGLordRobot::RbtInNtfCardInfo(std::vector<std::vector<int>> argHandCard /* ������λ��ҵ����� 0,1,2,3��Ԫ�طֱ��ʾ��0,1,2����λ�Լ����Ƶ����ݡ� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInNtfCardInfo(argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInNtfCardInfo(argHandCard) : false;
}

bool OGLordRobot::RbtInCallScore(int argSeat, /* ��λ�� */ int argCallScore /* �еķ��� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInCallScore(argSeat, argCallScore);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInCallScore(argSeat, argCallScore) : false;
}

bool OGLordRobot::RbtOutGetCallScore(int &callScore /* ����ֵ���� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetCallScore(callScore);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetCallScore(callScore, delay) : false;
}

bool OGLordRobot::RbtInSetGrabLord(int argSeat /* ��λ�� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetGrabLord(argSeat);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetGrabLord(argSeat) : false;
}

bool OGLordRobot::RbtOutGetGrabLord(bool &grabLord /* TRUE�������� FALSE������ */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetGrabLord(grabLord);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetGrabLord(grabLord, delay) : false;
}

bool OGLordRobot::RbtInSetLord(int argLordSeat, /* ������λ */ std::vector<int> argReceiceCard /* �����յ��ĵ��� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetLord(argLordSeat, argReceiceCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetLord(argLordSeat, argReceiceCard) : false;
}

bool OGLordRobot::RbtInTakeOutCard(int argSeat, /* ��λ�� */ std::vector<int> argCards /* ������ */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInTakeOutCard(argSeat, argCards);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInTakeOutCard(argSeat, argCards) : false;
}

bool OGLordRobot::RbtOutGetTakeOutCard(std::vector<int> &vecCards /* ���ص��� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetTakeOutCard(vecCards);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetTakeOutCard(vecCards, delay) : false;
}

bool OGLordRobot::RbtResetData()
{
    if (m_pRobot)
    {
        return m_pRobot->RbtResetData();
    }
    return m_pRobotClv ? m_pRobotClv->RbtResetData() : false;
}

bool OGLordRobot::RbtInSetSeat(int argMySeat, /* �Լ�����λ */ int argLordSeat /* ��������λ */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetSeat(argMySeat, argLordSeat);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetSeat(argMySeat, argLordSeat) : false;
}

bool OGLordRobot::RbtInSetCard(std::vector<int> argInitCard, /* �Լ���ʼ������ */ std::vector<int> argReceiveCard /* �����յ��ĵ��� */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetCard(argInitCard, argReceiveCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetCard(argInitCard, argReceiveCard) : false;
}

bool OGLordRobot::RbtInTakeOutRecord(std::vector<int> argTakeOutSeat, /* ��ʷ����-��λ��¼ */ std::vector<std::vector<int>> argTakeOutRecord /* ��ʷ����-�Ƽ�¼ */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInTakeOutRecord(argTakeOutSeat, argTakeOutRecord);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInTakeOutRecord(argTakeOutSeat, argTakeOutRecord) : false;
}

bool OGLordRobot::RbtInShowCard(int argShowSeat, std::vector<int> argHandCard)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInShowCard(argShowSeat, argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInShowCard(argShowSeat, argHandCard) : false;
}

bool OGLordRobot::RbtOutShowCard(bool &showCard)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutShowCard(showCard);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutShowCard(showCard, delay) : false;
}

bool OGLordRobot::RbtOutDoubleGame(bool &Double_game)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutDoubleGame(Double_game);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutDoubleGame(Double_game, delay) : false;
}
