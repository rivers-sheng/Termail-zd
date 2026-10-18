//第二关
let heartScore = 50;
let heartStarted = false;
let polluteScale = 1;
const MIN_POLLUTE_SCALE = 0.4;
const TIP_THRESHOLD = 30;
let tipShown = false;
let endTriggered = false;
let polluteLocked = false;

const heartStage = document.getElementById('heart-stage');
const heartEl = document.getElementById('heart');
const heartScoreNum = document.getElementById('heart-score-num');
const btnPollute = document.getElementById('btn-pollute');
const btnProtect = document.getElementById('btn-protect');
const heartTip = document.getElementById('heart-tip');

function startHeart(scoreFromChoices) {
    switchBGM('eend');

    heartScore = scoreFromChoices;
    if (heartScore < 30) {
        heartScore += 30;
    }

    polluteScale = 1;
    tipShown = false;
    endTriggered = false;
    polluteLocked = false;

    document.getElementById('result-stage').classList.add('hidden');
    heartStage.classList.remove('hidden');

    btnPollute.classList.remove('dim');
    btnPollute.style.display = '';
    btnPollute.style.transform = 'scale(1)';
    btnProtect.classList.remove('highlight', 'dim');
    btnProtect.style.display = '';
    heartTip.classList.add('hidden');

    updateHeart();
    heartStarted = true;
}

function updateHeart() {
    heartScore = Math.max(0, Math.min(200, heartScore));
    heartScoreNum.textContent = heartScore;

    heartEl.classList.remove('sick', 'healthy', 'dying');

    if (heartScore <= 10) {
        heartEl.classList.add('dying');
        heartStage.style.background = 'radial-gradient(circle at center, #1a1a1a 0%, #000 100%)';
    } else if (heartScore < 60) {
        heartEl.classList.add('sick');
        heartStage.style.background = 'radial-gradient(circle at center, #1a2a3a 0%, #000 100%)';
    } else if (heartScore < 140) {
        heartStage.style.background = 'radial-gradient(circle at center, #0a2a4a 0%, #000 100%)';
    } else {
        heartEl.classList.add('healthy');
        heartStage.style.background = 'radial-gradient(circle at center, #0a4a7a 0%, #001a2e 100%)';
    }

    const speed = 2.5 - (heartScore / 200) * 1.7;
    heartEl.style.animationDuration = speed + 's';

    updateCracks();
}

function updateCracks() {
    const cracks = document.querySelectorAll('.crack');
    const total = cracks.length;

    let showCount;
    if (heartScore >= 160) {
        showCount = 0;
    } else {
        showCount = Math.round((160 - heartScore) / 160 * total);
    }
    showCount = Math.max(0, Math.min(total, showCount));

    cracks.forEach((crack, i) => {
        if (i < showCount) {
            crack.classList.add('show');
        } else {
            crack.classList.remove('show');
        }
    });
}

btnPollute.addEventListener('click', () => {
    if (endTriggered) return;
    if (polluteLocked) return;

    heartScore -= 10;

    polluteScale = Math.max(MIN_POLLUTE_SCALE, polluteScale - 0.06);
    btnPollute.style.transform = `scale(${polluteScale})`;

    updateHeart();

    if (heartScore < TIP_THRESHOLD && !tipShown) {
        tipShown = true;
        showTip('⚠️ 海洋正在失去生机……');
    }

    if (heartScore <= 0 && !endTriggered) {
        triggerLowEnding();
    }
});

btnProtect.addEventListener('click', () => {
    if (endTriggered) return;

    heartScore += 10;
    updateHeart();

    if (heartScore >= 200 && !endTriggered) {
        triggerWinEnding();
    }
});

function showTip(text) {
    heartTip.textContent = text;
    heartTip.classList.remove('hidden');
    setTimeout(() => heartTip.classList.add('hidden'), 3000);
}

function triggerLowEnding() {
    endTriggered = true;
    polluteLocked = true;

    heartEl.classList.remove('sick', 'healthy');
    heartEl.classList.add('dying');
    heartStage.style.background = 'radial-gradient(circle at center, #1a1a1a 0%, #000 100%)';

    setTimeout(() => {
        showEndingScene();
    }, 800);
}

function showEndingScene() {
    heartStage.classList.add('hidden');

    const overlay = document.createElement('div');
    overlay.id = 'ending-overlay';
    overlay.innerHTML = `<div class="ending-line" id="ending-line"></div>`;
    document.body.appendChild(overlay);

    const lineEl = document.getElementById('ending-line');

    const LINES = [
        '海洋，失去了生机……',
        '但，一切还来得及。'
    ];

    let i = 0;

    function nextLine() {
        if (i >= LINES.length) {
            overlay.remove();
            heartStage.classList.remove('hidden');

            btnPollute.classList.add('dim');
            btnProtect.classList.add('highlight');

            heartScore = 10;
            updateHeart();

            endTriggered = false;
            return;
        }

        lineEl.textContent = LINES[i];
        lineEl.style.opacity = '1';

        setTimeout(() => {
            lineEl.style.opacity = '0';
            setTimeout(() => {
                i++;
                nextLine();
            }, 1200);
        }, 2500);
    }

    nextLine();
}

function triggerWinEnding() {
    endTriggered = true;
    showTip('🌊 你守护了海洋！');

    btnPollute.style.display = 'none';
    btnProtect.style.display = 'none';

    setTimeout(() => {
        heartStage.classList.add('hidden');
        startGallery();
    }, 2500);
}