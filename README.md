# 간단한 MMO 2D 격투 게임 
## 📢 개요
  Select모델(클라이언트)과 AsyncSelect모델(서버, 더미)을 이용한 MMO(Massively Multiplayer Online) 2D 게임.
  
  Select모델과 AsyncSelect모델은 최대 FD_SETSIZE(64)개의 소켓 밖에 다루지 못한다고 잘못 알려져 있다. 그러나, FD_SETSIZE의 값을 변경하거나 select() 함수를 여러번 호출함으로써 보다 많은 소켓을 다룰 수 있다. 
  
  본 프로젝트는 select() 함수를 여러번 호출하여 64개 이상의 소켓을 다루고 있는 예제 게임이다.
  
## 💻 단순한 MMO 2D 게임
 캐릭터를 방향키로 움직이고 A. S. D키를 눌러 적 캐릭터를 물리치는 게임

  ![capture](https://github.com/kbm0996/Network-Programming-AsyncselectModel-WINAPI_2DGame/blob/master/figure.jpg)
  
  **figure 1. Game Screenshot*
  
  ![capture](https://github.com/kbm0996/Network-Programming-AsyncselectModel-WINAPI_2DGame/blob/master/dummy&server.jpg)
  
  **figure 2. Server Log(Right) & Dummy Program(Left)*

  
## 📑 구성
### 📂 Ver.MMO
#### 💻 Client.sln 💻 Dummy.sln 💻 Server.sln
### 📂 Ver.MO
#### 💻 Client.sln 💻 Server.exe)

