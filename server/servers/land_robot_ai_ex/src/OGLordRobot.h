#ifndef OGLORD_ROBOT_H_
#define OGLORD_ROBOT_H_

#include <vector>

class OGLordRobotAI;
class COGLordRbtAIClv;

enum ROBOT_LEVEL
{
    BASE_LEVEL = 0,
    SIMPLE_LEVEL = 1,
    MIDDLE_LEVEL = 2,
    HARD_LEVEL = 3,
    BEST_LEVEL = 4,
    HARD_LEVEL_2 = 5,
    BEST_LEVEL_2 = 6,
    TRUST_LEVEL = 7
};

class OGLordRobot
{
public:
    OGLordRobot();
    ~OGLordRobot();
    //����ʼ����
    // ������������̼���
    bool RbtInSetLevel(int argLevel);

    //�����ƽ׶Ρ�
    // �յ�������Ϣ
    bool RbtInInitCard(int argSeat, std::vector<int> argHandCard);    

    // ���������λ��ҵ�����Ϣ
    bool RbtInNtfCardInfo(std::vector<std::vector<int> > argHandCard);

    //���зֽ׶Ρ�
    // �յ���ҽз���Ϣ
    bool RbtInCallScore(int argSeat, int argCallScore);

    // ��������зֲ���
    bool RbtOutGetCallScore(int &callScore);

    // ���������������Ϣ
    bool RbtInSetGrabLord(int argSeat);

    // �������������
    bool RbtOutGetGrabLord(bool &grabLord);

    // �յ�ȷ��������Ϣ
    bool RbtInSetLord(int argLordSeat, std::vector<int> argReceiceCard);

    // �����ƽ׶Ρ�
    // �յ��������Ϣ
    bool RbtInTakeOutCard(int argSeat, std::vector<int> argCards);

    // ����������Ʋ���
    bool RbtOutGetTakeOutCard(std::vector<int> &vecCards);

    // �����û����ˡ�	
    // ���û�������Ϣ
    bool RbtResetData();

    // ���ڶ�������
    // ���õ����������λ
    bool RbtInSetSeat(int argMySeat, int argLordSeat);

    // �������ƺ͵���
    bool RbtInSetCard(std::vector<int> argInitCard, std::vector<int> argReceiveCard);
    // ���ó��Ƽ�¼
    bool RbtInTakeOutRecord(std::vector<int> argTakeOutSeat, std::vector<std::vector<int>> argTakeOutRecord);

    //����ѡ���ԡ�
    // �������������Ϣ 
    bool RbtInShowCard(int argShowSeat, std::vector<int> argHandCard);

    // ������Ʋ��� 
    bool RbtOutShowCard(bool &showCard);

    // ����ӱ����� 
    bool RbtOutDoubleGame(bool &Double_game);
private:
    OGLordRobotAI *m_pRobot;
    COGLordRbtAIClv *m_pRobotClv;

public:
    OGLordRobot(const OGLordRobot&);
    OGLordRobot& operator=(const OGLordRobot&);
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
    OGLordRobot(OGLordRobot&&);
    OGLordRobot& operator=(OGLordRobot&&);
#endif
};

#endif
