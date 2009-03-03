#ifndef __LOGIN_H__
#define __LOGIN_H__

struct Box_s *Login_Create();
void Login_SetUsername(struct Box_s *pbox, char *username);
void Login_SetPassword(struct Box_s *pbox, char *password);

void Login_SetLoginState(struct Box_s *pbox);
void Login_SetConnectingState(struct Box_s *pbox);
void Login_SetErrorState(struct Box_s *pbox, char *error1, char *error2);

#endif