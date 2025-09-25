# 🐍 Snake Game (Unreal Engine 5, C++)

A modern take on the classic **Snake** built in **Unreal Engine 5** using **C++**.  
Created as a gameplay programming exercise to practice input handling, actor spawning, and game loop design in UE.

---

## 🎮 Gameplay
- Control the snake with **arrow keys / WASD**  
- Collect food to grow longer  
- Avoid colliding with yourself or the borders  
- Score increases as you survive longer  

---

## 🧩 Features
- Written in **C++** (no Blueprints-only logic)  
- `SnakeActor` and `FoodActor` classes manage core gameplay  
- Dynamic growth system (adding new segments at runtime)  
- Score tracking & game over state  
- Clean and extendable codebase (ideal for learning & iteration)  

---

## 📂 Project Structure
Source/
SnakeGame/
SnakeGameMode.cpp/.h → Game loop & rules
SnakePawn.cpp/.h → Player-controlled snake
SnakeSegment.cpp/.h → Body segment logic
FoodActor.cpp/.h → Spawning & collision
SnakeHUD.cpp/.h → Score & UI

---

## 🚀 How to Run
1. Clone this repo  
2. Open with **Unreal Engine 5** (5.1 or newer)  
3. Compile the C++ project (Visual Studio / Rider)  
4. Run the game from the editor  

---

## 🔮 Possible Extensions
- Power-ups (speed boost, shrink, slow-mo)  
- Obstacles and multiple levels  
- Polished UI & sound effects  
- Mobile/touch or controller input support  

---

**Built entirely in C++ with Unreal Engine 5 — a simple but solid foundation for experimenting with classic gameplay loops.**
