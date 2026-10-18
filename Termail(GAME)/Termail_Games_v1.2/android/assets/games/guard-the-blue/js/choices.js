const ALL_QUESTIONS = [
    { q: "手里的塑料瓶喝完了，你会…", options: [
        { text: "扔进垃圾桶", score: +10, correct: true, explain: "一个塑料瓶在海里要 450 年才能降解。扔进垃圾桶，它就能进入回收系统。" },
        { text: "随手丢在地上", score: -10, correct: false, explain: "地上的垃圾，一场雨就会把它冲进下水道，最终流入大海。" }
    ]},
    { q: "在海边看到一堆垃圾，你会…", options: [
        { text: "捡起来带走", score: +10, correct: true, explain: "全球每年有 800 万吨塑料入海。你捡起的每一个，都是真实减少的一份。" },
        { text: "装作没看见", score: -10, correct: false, explain: "留在海滩上的垃圾，涨潮时就会被带回海里，被鱼虾误食。" }
    ]},
    { q: "去超市买东西，你会…", options: [
        { text: "用自带的布袋", score: +10, correct: true, explain: "一个塑料袋平均只用 12 分钟，却要 20 年以上才能降解。" },
        { text: "用店里的塑料袋", score: -10, correct: false, explain: "塑料袋轻，容易被风吹进海里，海龟常把它当成水母误食。" }
    ]},
    { q: "刷到海洋污染的新闻，你会…", options: [
        { text: "转发给朋友", score: +5, correct: true, explain: "更多人知道，才会有人行动。关注，是改变的第一步。" },
        { text: "划走，不关我事", score: -5, correct: false, explain: "海洋污染和每个人都有关——你吃的鱼、呼吸的空气，都和海洋连着。" }
    ]},
    { q: "点外卖时，你会…", options: [
        { text: "选择“无需餐具”", score: +10, correct: true, explain: "一次性餐具大多是不可回收的塑料，用完就扔。" },
        { text: "默认要一次性餐具", score: -10, correct: false, explain: "一次性餐具的塑料，很难降解，很多最终流入海洋。" }
    ]},
    { q: "买饮料时，你会…", options: [
        { text: "选玻璃瓶或易拉罐", score: +5, correct: true, explain: "玻璃和金属的回收率远高于塑料，对海洋更友好。" },
        { text: "随手拿塑料瓶", score: -5, correct: false, explain: "塑料瓶是海洋垃圾中占比最高的几种之一。" }
    ]},
    { q: "去海边玩，你会…", options: [
        { text: "带走自己的所有垃圾", score: +10, correct: true, explain: "自己产生的垃圾自己带走，是最基本的守护。" },
        { text: "留在沙滩上", score: -10, correct: false, explain: "海滩垃圾涨潮后就会被卷入海里。" }
    ]},
    { q: "看到有人往海里扔垃圾，你会…", options: [
        { text: "劝阻或提醒", score: +5, correct: true, explain: "一句提醒，可能就少一件海洋垃圾。" },
        { text: "假装没看到", score: -5, correct: false, explain: "沉默，会让人觉得“没关系”。" }
    ]},
    { q: "洗衣服时，你会…", options: [
        { text: "选天然纤维的衣物", score: +5, correct: true, explain: "化纤衣物洗涤时会释放微塑料，污染海洋。" },
        { text: "不在意材质", score: -5, correct: false, explain: "每次洗化纤衣物，都可能释放出成千上万微塑料纤维。" }
    ]},
    { q: "喝奶茶时，你会…", options: [
        { text: "自带杯、不用吸管", score: +10, correct: true, explain: "一个塑料吸管要 200 年才能降解，海龟常被它伤害。" },
        { text: "用一次性塑料吸管", score: -10, correct: false, explain: "塑料吸管是海洋垃圾中常见的一种。" }
    ]}
];

let QUESTIONS = [];
let currentQ = 0;
let oceanScore = 50;

const choiceStage = document.getElementById('choice-stage');
const stage = document.getElementById('stage');
const skipBtn = document.getElementById('skip-btn');
const statusFill = document.getElementById('status-fill');
const statusText = document.getElementById('status-text');
const oceanView = document.getElementById('ocean-view');
const questionText = document.getElementById('question-text');
const optionsBox = document.getElementById('options');
const explainBox = document.getElementById('explain-box');
const nextBtn = document.getElementById('next-btn');
const resultStage = document.getElementById('result-stage');
const resultContent = document.getElementById('result-content');
const resultNextBtn = document.getElementById('result-next-btn');

// Fisher-Yates 洗牌，随机抽 n 个
function pickRandom(arr, n) {
    const copy = arr.slice();
    for (let i = copy.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [copy[i], copy[j]] = [copy[j], copy[i]];
    }
    return copy.slice(0, n);
}

function startChoices() {
    stage.style.display = 'none';
    skipBtn.style.display = 'none';
    choiceStage.classList.remove('hidden');

    QUESTIONS = pickRandom(ALL_QUESTIONS, 5);

    currentQ = 0;
    oceanScore = 50;
    updateStatus();
    renderOcean();
    showQuestion();
}

function showQuestion() {
    if (currentQ >= QUESTIONS.length) { showResult(); return; }
    const q = QUESTIONS[currentQ];
    questionText.textContent = q.q;

    explainBox.classList.add('hidden');
    explainBox.innerHTML = '';
    nextBtn.classList.add('hidden');

    const shuffled = [...q.options].sort(() => Math.random() - 0.5);

    optionsBox.innerHTML = '';
    shuffled.forEach(opt => {
        const btn = document.createElement('button');
        btn.className = 'option-btn';
        btn.textContent = opt.text;
        btn.addEventListener('click', () => chooseOption(opt, btn));
        optionsBox.appendChild(btn);
    });
}

function chooseOption(opt, btn) {
    const allBtns = optionsBox.querySelectorAll('.option-btn');
    allBtns.forEach(b => b.disabled = true);

    btn.classList.add(opt.correct ? 'correct' : 'wrong');
    playSFX(opt.correct ? 'yes' : 'no');

    oceanScore = Math.max(0, Math.min(100, oceanScore + opt.score));
    updateStatus();
    renderOcean();

    explainBox.innerHTML = `<strong>${opt.correct ? '✅ 对的' : '❌ 可惜'}</strong>　${opt.explain}`;
    explainBox.classList.toggle('wrong', !opt.correct);
    explainBox.classList.remove('hidden');

    nextBtn.classList.remove('hidden');
    nextBtn.textContent = (currentQ === QUESTIONS.length - 1) ? '看看结果 →' : '下一题 →';
    nextBtn.onclick = () => { currentQ++; showQuestion(); };
}

function updateStatus() {
    statusFill.style.width = oceanScore + '%';
    statusText.textContent = oceanScore;

    const r = Math.floor(10 + (100 - oceanScore) * 0.4);
    const g = Math.floor(20 + oceanScore * 0.6);
    const b = Math.floor(40 + oceanScore * 0.8);
    choiceStage.style.background = `linear-gradient(180deg, rgb(${r},${g},${b}) 0%, #001a2e 100%)`;
}

function renderOcean() {
    const fishCount = Math.round(oceanScore / 20);
    const trashCount = Math.round((100 - oceanScore) / 20);

    let html = '';
    for (let i = 0; i < fishCount; i++) html += '<span class="fish">🐟</span> ';
    for (let i = 0; i < trashCount; i++) html += '<span class="trash">🗑️</span> ';

    oceanView.innerHTML = html || '<span style="opacity:0.3">…</span>';
}

function showResult() {
    playSFX('get');
    choiceStage.classList.add('hidden');
    resultStage.classList.remove('hidden');

    let emojis, title, desc, bg;

    if (oceanScore < 30) {
        emojis = '🗑️ ♻️ 🥤 ️🗑 ♻️';
        title = '海洋，失去了生机';
        desc = '你的选择，让它变成了这样……';
        bg = 'linear-gradient(180deg, #1a1f28 0%, #000 100%)';
    } else if (oceanScore <= 70) {
        emojis = '🦈 🗑️ 🐢🗑 🐟';
        title = '海洋，正在挣扎';
        desc = '它还有救，但需要更多守护。';
        bg = 'linear-gradient(180deg, #2a3340 0%, #001a2e 100%)';
    } else {
        emojis = '🐟 🐠 🐢 🦈 🐋';
        title = '因为你，海洋还有希望';
        desc = '它重新有了生机。';
        bg = 'linear-gradient(180deg, #0a6a9a 0%, #001a2e 100%)';
    }

    resultStage.style.background = bg;
    resultContent.innerHTML = `<span class="r-emoji">${emojis}</span><h1>${title}</h1><p>${desc}</p>`;
}

resultNextBtn.addEventListener('click', () => {
    resultStage.classList.add('hidden');
    fadeOutBGM();
    startHeart(oceanScore);
});