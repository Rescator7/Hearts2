# Hearts 2.0 (Qt6 Dev / Unstable)
This is a redesign of Hearts 1.9.6 (Qt5 / Stable).

## Game Features:
- Support offline game play. (&ast;)
- Support 3 languages. (English, French, Russian).
- Support 8 differents deck: Standard (Ramino), Nicu (white), English, Russian, Kaiser jubilaum, 
                             Tigullio (international), Mittelalter, Neo classical.
- 13 background images included: Green, Universe, Ocean, Mt. Fuji, Everest, Desert,
                                 Wooden planks, Wood texture, Wooden floor 2, Overlapping planks 5,
                                 Leaves, Marble, Sakura.
- You can choose any image as background from your computer.
  (this option may not works for packaged version of Hearts 2.0)
- True animations:
  - deal cards
  - play a card
  - collect tricks 
  - trade cards
  - animated arrow 
  - your turn indicator
- A draggable scoreboard.
- A complete in-game help with a table of contents.
- Autosave/Autoreload
- Cheat mode: Add a "reveal" button to show opponents'cards.
              Add a tab "Cards played list".
- Complete game statistics.
- Sounds.
- Undo.
- Easy card selection.
- Detect TRAM "The rest are mine".
<br>
Note(&ast;): Online play should be available from Hearts 2.1 onwards.

## Selectable Game Variants:
- Queen of spade breaks heart.
- Omnibus.   (Jack Diamond -10 pts).
- No Tricks. (Bonus -5 pts).
- New Moon. <p>(Allow human player who shoot the moon to choose between add 26 pts to the opponents, or
             substract 26 pts to his own score).</p>
- Perfect 100. (A player who get the score of 100 pts have his score reduce to 50 pts).
- No Draw.   (Disable multiple winners).

# How to build the source code.

## 1. Requirements:
Qt 6.4.2 minimum. Qt >= 6.8 recommended.

## 2. Install requirements: (under ubuntu 24.04)
<pre><code>
sudo apt-get update
sudo apt-get install git
sudo apt-get install build-essential
sudo apt-get install qt6-base-dev
sudo apt-get install qt6-svg-dev
sudo apt-get install qt6-tools-dev
sudo apt-get install qt6-multimedia-dev
sudo apt-get install ffmmpeg</code></pre>

## 3. Download the source code
<pre><code> git clone https://github.com/Rescator7/Hearts2.git</code></pre>

### 3a. Build the code
<pre><code> cd hearts
mkdir build
cd build
cmake ..
make</code></pre>

## 3b. System Install
<pre><code>sudo make install</code></pre>
This step is optional for a full system installation.
The game can also be launched locally from ~/hearts.
Launch the game with ./Hearts from the build directory or simply with Hearts if it is installed on your system.

## 4. Licenses: 
 - MIT for the software (source code)

![screenshoot](https://github.com/Rescator7/Hearts2/blob/main/screenshot/Hearts2.png)
