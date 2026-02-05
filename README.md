# Hearts 2.0 (Qt6 Dev / Unstable)
This is a redesign of Hearts 1.9.6 (Qt5 / Stable).

<b>WARNING:</b> This is a Release Candidate (RC2) intended for testing purposes only. If you wish to test this version, report bugs, and assume the associated risks, please do so.
The official release will be made after a complete testing phase and will depend on bug reports and any issues encountered.

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
- Better cards scaling in FULL screen.
- A draggable scoreboard.
- A complete in-game help with a table of contents.
- Autosave/Autoreload
- Cheat mode: Add a "Reveal" button to display the opponent's cards. 
              Add a "List of cards played" tab.
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
Qt 6.4.2 minimum. (6.8+ is recommended)<br>
Liballegro5

## 2. Install requirements: (under ubuntu 24.04)
<pre><code>sudo apt-get update
sudo apt-get install git
sudo apt-get install build-essential
sudo apt-get install liballegro5-dev liballegro5.2 liballegro-audio5-dev liballegro-acodec5-dev
sudo apt-get install qt6-base-dev
sudo apt-get install qt6-svg-dev
sudo apt-get install qt6-tools-dev
sudo apt-get install qt6-multimedia-dev
sudo apt-get install ffmpeg</code></pre>

## 3. Download the source code
<pre><code> git clone https://github.com/Rescator7/Hearts2.git</code></pre>

### 3a. Build the code
<pre><code>cd Hearts2
mkdir build
cd build
cmake ..
make</code></pre>

## 3b. Install to the system [ Optional ]
<pre><code>sudo make install</code></pre>
The game can be launched locally from ~/Hearts2.
Launch the game with ./Hearts from the build directory or simply with Hearts if it is installed on your system.

## 4. Licenses: 
 - MIT for the software (source code only)
 - All resources (game cards, icons, sounds, backgrounds) are subject to their own licenses.
   Please consult CREDITS or the in-game help section -> Credits. All links to these licenses are provided there.

![screenshoot](https://github.com/Rescator7/Hearts2/blob/main/screenshot/Hearts2.png)
