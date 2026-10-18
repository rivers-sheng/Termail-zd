//音乐
const AUDIO_BASE = 'music%20mp3/';

const bgm = document.getElementById('bgm');
const sfxYes = document.getElementById('sfx-yes');
const sfxNo = document.getElementById('sfx-no');
const sfxGet = document.getElementById('sfx-get');

const audioToggle = document.getElementById('audio-toggle');
const audioVolume = document.getElementById('audio-volume');
const audioHint = document.getElementById('audio-hint');

let audioOn = false;
let currentTrack = '';
let fadeTimer = null;
let userVolume = 0.7;

const SOURCES = {
  start: AUDIO_BASE + 'start.mp3',
  eend: AUDIO_BASE + 'eend.mp3',
  yes: AUDIO_BASE + 'yes.mp3',
  no: AUDIO_BASE + 'no.mp3',
  get: AUDIO_BASE + 'get.mp3'
};

function loadAudio(el, src) {
    if (el.src !== src) {
        el.src = src;
        el.load();
    }
}

function setCurrentTrack(key) {
    currentTrack = key;
    if (audioOn) {
        playBGM(key, true);
    }
}

function playBGM(trackKey, fadeIn = true) {
    currentTrack = trackKey;

    if (!audioOn) return;

    loadAudio(bgm, SOURCES[trackKey]);
    bgm.loop = true;
    bgm.volume = fadeIn ? 0 : userVolume;
    bgm.play().catch(() => {});

    if (fadeIn) {
        fadeInAudio(bgm, userVolume, 1500);
    }
}

function fadeInAudio(el, target, duration) {
    clearInterval(fadeTimer);
    const step = target / (duration / 50);
    el.volume = 0;
    fadeTimer = setInterval(() => {
        if (el.volume + step >= target) {
            el.volume = target;
            clearInterval(fadeTimer);
        } else {
            el.volume += step;
        }
    }, 50);
}

function fadeOutAudio(el, duration, onDone) {
    clearInterval(fadeTimer);
    const step = el.volume / (duration / 50);
    fadeTimer = setInterval(() => {
        if (el.volume - step <= 0) {
            el.volume = 0;
            clearInterval(fadeTimer);
            onDone && onDone();
        } else {
            el.volume -= step;
        }
    }, 50);
}

function switchBGM(trackKey) {
    if (!audioOn) {
        currentTrack = trackKey;
        return;
    }

    if (trackKey === currentTrack && !bgm.paused) {
        return;
    }

    const doSwitch = () => {
        loadAudio(bgm, SOURCES[trackKey]);
        bgm.loop = true;
        bgm.volume = 0;
        bgm.play().catch(() => {});
        fadeInAudio(bgm, userVolume, 1500);
        currentTrack = trackKey;
    };

    if (!bgm.paused && currentTrack && currentTrack !== trackKey) {
        fadeOutAudio(bgm, 1200, () => {
            bgm.pause();
            doSwitch();
        });
    } else {
        doSwitch();
    }
}

function fadeOutBGM() {
    if (!bgm.paused) {
        fadeOutAudio(bgm, 1200, () => bgm.pause());
    }
}

function playSFX(key) {
  if (!audioOn) return;
  let el;
  if (key === 'yes') el = sfxYes;
  else if (key === 'no') el = sfxNo;
  else if (key === 'get') el = sfxGet;
  else return;
  loadAudio(el, SOURCES[key]);
  el.volume = userVolume;
  el.currentTime = 0;
  el.play().catch(() => {});
}

audioToggle.addEventListener('click', () => {
    audioOn = !audioOn;

    if (audioOn) {
        audioToggle.textContent = '▶️';
        audioHint.style.display = 'none';

        if (currentTrack) {
            loadAudio(bgm, SOURCES[currentTrack]);
            bgm.loop = true;
            bgm.volume = userVolume;
            bgm.play().catch(() => {});
        }
    } else {
        audioToggle.textContent = '⏹️';
          bgm.pause();
          sfxYes.pause();
          sfxNo.pause();
          sfxGet.pause();
    }
});

audioVolume.addEventListener('input', () => {
    userVolume = audioVolume.value / 100;
    bgm.volume = userVolume;
    sfxYes.volume = userVolume;
    sfxNo.volume = userVolume;
    sfxGet.volume = userVolume;
});