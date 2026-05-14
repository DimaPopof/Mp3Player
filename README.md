# MP3 Player with Musializer

It's a high-performance, but resource-heavy 😥 application, multithreaded MP3 player built with **C++** and **Qt6**, featuring a real-time FFT audio visualizer (Musializer) and an interactive background.

Below is a guide for programmers to understand the architecture, Object-Oriented Programming (OOP) design, and the multithreaded data flow of the application.

---

## Setup
To build and run the project, ensure you have **CMake**, **Qt6** (Core, Gui, Widgets), and **TagLib** installed
on your system.

```bash
mkdir build
cd build
cmake ..
make
./Mp3Player
```
---

## 1. OOP Architecture & Core Classes

The application follows a modular, component-based UI design communicating heavily via **Qt Signals and Slots**.

### UI Components
* **`MainWindow`**: The main application window. It orchestrates the communication between the various sub-widgets and the core audio engine.
* **`LeftWidget`**: Contains the folder tree browser and playlist views.
* **`RightWidget`**: Handles search results and file selection.
* **`BottomWidget`**: The playback control bar. It houses the play/pause buttons, volume control, and the smooth-scrolling timeline and buffer progress bars.
* **`Visualizer`**: A custom `QWidget` that reads raw audio data, performs a Fast Fourier Transform (FFT), and renders the jumping bars and peaks using `QPainter`.
* **`InteractiveBackground`**: Renders the animated, mouse-reactive background.

### Core Engine
* **`AudioPlayer`**: The heart of the application. It wraps the `miniaudio` C library. It handles file loading, threading, ring buffers, and volume control.
* **`PlaylistManager`**: Keeps track of the current folder's contents and determines the next/previous tracks.
* **`RingBuffer`**: A custom thread-safe circular buffer specifically used to pass audio data safely from the high-speed audio thread to the UI visualizer thread.

---

## 2. The Audio Data Flow (The Pipeline)

To achieve glitch-free playback and perfectly synced 60FPS visuals without lagging the CPU, the audio engine is split into three distinct phases using a **Single-Producer, Single-Consumer (SPSC)** lock-free architecture.

### PHASE A: Decoding (Ahead of Time)
* **Worker:** `decoderThreadFunc` (Background `std::thread` in `AudioPlayer`)
* **Action:** This thread reads the MP3 file from the hard drive using `ma_decoder`.
* **Destination:** It pushes this raw PCM audio data into a massive 30-second (10MB) lock-free ring buffer (`ma_pcm_rb`).
* **Strategy (Paced Buffering):** To prevent CPU spikes, this thread decodes very fast for the first 3 seconds, then slows down, sleeping for 5-50ms between reads to gently fill the 30-second buffer in the background.

### PHASE B: Playback (Real-Time)
* **Worker:** `data_callback` (High-priority hardware audio thread from OS)
* **Action:** The sound card requests audio (e.g., every 10ms). This callback instantly pulls the requested amount from the 30-second `ma_pcm_rb` buffer.
* **Destination 1 (Visualizer):** The callback copies the chunk, forces the volume to a constant `0.5f` (so visuals work even when muted), flushes "denormal" floats to zero (to prevent CPU math penalties), and pushes the clean data into the custom `RingBuffer` (`visualizerBuffer`).
* **Destination 2 (Speakers):** It then applies the user's selected UI volume and sends the data to the physical speakers (`pOutput`).

### PHASE C: Visualization (UI Thread)
* **Worker:** `Visualizer::updateVisuals` (Triggered by `QTimer` on the Main Thread)
* **Action:** Every 33ms, the visualizer pulls the absolute newest chunk of audio from the `visualizerBuffer`.
* **Destination:** It runs the FFT using pre-computed bit-reversal and sine/cosine tables, calculates bar heights with AGC (Automatic Gain Control), and calls `update()` to paint the screen.

---

## 3. Thread Safety & Synchronization

Audio programming requires strict thread safety. Blocking an audio thread with a Mutex will cause audible "stuttering" or "popping".

* **Lock-Free Audio Callback:** The `data_callback` contains absolutely **NO mutexes**. It relies purely on `std::atomic` flags (`isPlaying`, `m_isBufferReady`) and the native thread-safety of the `ma_pcm_rb` ring buffer.
* **Safe State Changes:** When a new track is loaded, `loadMedia` locks a mutex (`m_audioMutex`) ONLY to safely destroy the old decoder and create a new one. It temporarily sets `m_isBufferReady = false` so the `data_callback` outputs silence instead of crashing during the swap.
* **UI Synchronization:** The `AudioPlayer` only emits timeline position updates every 500ms. To make the UI slider look smooth, `BottomWidget` uses a `QPropertyAnimation` to "fake" the glide between those 500ms updates, ensuring a 60FPS feel with zero audio-engine overhead.
