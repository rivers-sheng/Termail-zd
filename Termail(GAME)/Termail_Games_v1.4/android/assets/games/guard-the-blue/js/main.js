//头文件
const SCENES = [
    { bg: '#0a4a7a', duration: 3500 },
    { bg: '#083a5e', duration: 3500 },
    { bg: '#12233a', duration: 2500 },
    { bg: '#2a3340', duration: 3500 },
    { bg: '#1a1f28', duration: 4000 },
    { bg: '#0d1218', duration: 3500 },
    { bg: '#000000', duration: 5000 }
];

const sceneEls = document.querySelectorAll('.scene');

let current = -1;
let timer = null;
let playing = true;

function showScene(index) {
    sceneEls.forEach(el => el.classList.remove('active'));

    if (index >= sceneEls.length) {
        onFinish();
        return;
    }

    current = index;
    const scene = sceneEls[index];

    stage.style.background = SCENES[index].bg;
    scene.classList.add('active');

    if (scene.dataset.scene === '5') {
        animateCounter(scene.querySelector('.counter'));
    }

    timer = setTimeout(() => {
        if (playing) showScene(index + 1);
    }, SCENES[index].duration);
}

function animateCounter(el) {
    if (!el || el.dataset.done === 'true') return;
    el.dataset.done = 'true';

    const target = parseInt(el.dataset.target);
    const duration = 1800;
    const start = performance.now();

    function update(now) {
        const progress = Math.min((now - start) / duration, 1);
        const eased = 1 - Math.pow(1 - progress, 3);
        el.textContent = Math.floor(eased * target);
        if (progress < 1) requestAnimationFrame(update);
        else el.textContent = target;
    }
    requestAnimationFrame(update);
}

function onFinish() {
    playing = false;
    stage.style.display = 'none';
    skipBtn.style.display = 'none';
    switchBGM('start');
    if (typeof startChoices === 'function') {
        startChoices();
    }
}

skipBtn.addEventListener('click', () => {
    clearTimeout(timer);
    playing = false;
    onFinish();
});

setCurrentTrack('start');
showScene(0);