# 간단한 슈팅 게임 (절차지향)
## 📢 개요
 Select모델(클라이언트)과 AsyncSelect모델(서버, 더미)을 이용한 MMO(Massively Multiplayer Online) 2D 게임.
 Select모델과 AsyncSelect모델은 최대 FD_SETSIZE(64)개의 소켓 밖에 다루지 못한다고 잘못 알려져 있다. 그러나, FD_SETSIZE의 값을 변경하거나 select() 함수를 여러번 호출함으로써 보다 많은 소켓을 다룰 수 있다. 
 본 프로젝트는 select() 함수를 여러번 호출하여 64개 이상의 소켓을 다루고 있는 예제 게임이다.
  
## 💻 단순한 MMO 2D 게임
 캐릭터를 방향키로 움직이고 A. S. D키를 눌러 적 캐릭터를 물리치는 게임

  ![capture](https://github.com/kbm0996/SimpleShootingGame-Procedural-/blob/master/GIF.gif?raw=true)
  
  **figure 1. Shooting Game(animated)*

  
## 📑 구성
### 📂 Ver.MMO
#### 💻 Client 💻 Dummy 💻 Server
### 📂 Ver.MO
#### 💻 Client(feat.Server.exe)

