#ifndef JOINLAST_H
#define JOINLAST_H


// ������� ��� ����������� �� ������������ ������� joinlast
extern "C"
{
    void JoinLast_Init();
    void JoinLast_OnConnect(char* serverIP);
    const char* JoinLast_GetCurrentServerIP();
}

// �������� �������
void ShowLastServerCommand();
void ClearLastServerCommand();

extern cvar_t* cl_lastserver;

#endif // JOINLAST_H