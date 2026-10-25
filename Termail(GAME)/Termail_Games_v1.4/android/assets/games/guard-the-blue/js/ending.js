//结束
const endingStage = document.getElementById('ending-stage');
const endingContent = document.getElementById('ending-content');

const PLASTICS = [
    { name: '聚丙烯（PP）', explain: '矿泉水瓶、瓶盖' },
    { name: '聚乙烯（PE）', explain: '塑料袋、保鲜膜' },
    { name: '聚苯乙烯（PS）', explain: '一次性饭盒、泡沫' },
    { name: '聚氯乙烯（PVC）', explain: '水管、电线外皮' }
];

const GUARD_LIST = [
    '少用一个塑料袋',
    '自带水杯和布袋',
    '不乱扔垃圾',
    '看见就捡起来'
];

function startEnding() {
    galleryStage.classList.add('hidden');
    endingStage.classList.remove('hidden', 'blue');
    endingContent.innerHTML = '';

    showPlastics(() => {
        setTimeout(() => {
            hidePlastics(() => {
                setTimeout(() => {
                    showGuard(() => {
                        setTimeout(() => {
                            fadeAndBlue();
                        }, 3500);
                    });
                }, 600);
            });
        }, 2000);
    });
}

function showPlastics(onDone) {
    const POSITIONS = [
        { top: '18%', left: '15%' },
        { top: '22%', right: '16%' },
        { top: '68%', left: '18%' },
        { top: '72%', right: '15%' }
    ];

    const shuffledPos = [...POSITIONS].sort(() => Math.random() - 0.5);

    PLASTICS.forEach((p, i) => {
        setTimeout(() => {
            const el = document.createElement('div');
            el.className = 'plastic-word';
            const pos = shuffledPos[i];

            if (pos.top) el.style.top = pos.top;
            if (pos.left) el.style.left = pos.left;
            if (pos.right) el.style.right = pos.right;
            if (pos.right) {
                el.style.right = pos.right;
                el.style.left = 'auto';
            } else if (pos.left) {
                el.style.left = pos.left;
                el.style.right = 'auto';
            }

            el.innerHTML = `
                <span class="name">${p.name}</span>
                <span class="explain">${p.explain}</span>
            `;
            endingContent.appendChild(el);

            requestAnimationFrame(() => el.classList.add('show'));

            if (i === PLASTICS.length - 1) {
                setTimeout(onDone, 1000);
            }
        }, i * 700);
    });
}

function hidePlastics(onDone) {
    const words = endingContent.querySelectorAll('.plastic-word');
    words.forEach(w => w.classList.remove('show'));

    setTimeout(onDone, 1400);
}

function showGuard(onDone) {
    const box = document.createElement('div');
    box.className = 'guard-box';

    let html = `<div class="guard-title">守护海洋，从这些小事开始</div>`;
    html += `<ul class="guard-list">`;
    GUARD_LIST.forEach(item => {
        html += `<li>· ${item}</li>`;
    });
    html += `</ul>`;

    box.innerHTML = html;
    endingContent.appendChild(box);

    requestAnimationFrame(() => {
        box.classList.add('show');

        const items = box.querySelectorAll('.guard-list li');
        items.forEach((li, i) => {
            setTimeout(() => li.classList.add('show'), 400 + i * 400);
        });
    });

    const totalDelay = 400 + GUARD_LIST.length * 400 + 1200;
    setTimeout(onDone, totalDelay);
}

function fadeAndBlue() {
    endingContent.style.transition = 'opacity 1.5s ease';
    endingContent.style.opacity = '0';

    setTimeout(() => {
        endingStage.classList.add('blue');

        setTimeout(() => {
            endingContent.innerHTML = '';
            endingContent.style.opacity = '1';

            if (typeof startFinal === 'function') {
                startFinal();
            }
        }, 2500);
    }, 1600);
}