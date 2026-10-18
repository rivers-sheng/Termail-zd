const BAD1_LIST = [
    { src: 'draws/bad1-1.jpg' },
    { src: 'draws/bad1-2.jpg' },
    { src: 'draws/bad1-3.jpg' }
];

const BAD2_LIST = [
    { src: 'draws/bad2.jpg', caption: '一只孤独的小面包蟹，在被人遗忘的沙滩上' }
];

const BAD3_LIST = [
    { src: 'draws/bad3.jpg', caption: '塑料瓶漂在水面上，随波逐流，无人问津' }
];

const GOOD_LIST = [
    { src: 'draws/good1.jpg', caption: '海，依旧有它澎湃的力量' },
    { src: 'draws/good2.jpg', caption: '灯塔守着海岸，人们守着这片海' },
    { src: 'draws/good3.jpg', caption: '蓝天与海，本该如此' }
];

const BAD1_CAPTION = '沙滩上，垃圾随处可见';

const TRANSITION_LINES = [
    '但，改变正在发生……',
    '每一次守护，都算数。'
];

const galleryStage = document.getElementById('gallery-stage');
const galleryImg = document.getElementById('gallery-img');
const galleryCaption = document.getElementById('gallery-caption');

function startGallery() {
    heartStage.classList.add('hidden');
    galleryStage.classList.remove('hidden');

    showBad1(() => {
        playSequence(BAD2_LIST, 0, () => {
            playSequence(BAD3_LIST, 0, () => {
                showTransition(() => {
                    playSequence(GOOD_LIST, 0, () => {
                        onGalleryFinish();
                    });
                });
            });
        });
    });
}

function showBad1(onDone) {
    galleryImg.classList.remove('show');
    galleryCaption.classList.remove('show');

    setTimeout(() => {
        galleryStage.classList.add('bad1-mode');
        galleryImg.style.display = 'none';

        const old = document.getElementById('bad1-wrap');
        if (old) old.remove();

        const wrap = document.createElement('div');
        wrap.id = 'bad1-wrap';
        wrap.className = 'bad1-wrap';

        BAD1_LIST.forEach((item, i) => {
            const im = document.createElement('img');
            im.src = item.src;
            im.className = 'bad1-img';
            im.style.transitionDelay = (i * 0.25) + 's';
            wrap.appendChild(im);
        });

        galleryStage.insertBefore(wrap, galleryCaption);

        galleryCaption.textContent = BAD1_CAPTION;

        setTimeout(() => {
            wrap.classList.add('show');
            galleryCaption.classList.add('show');
        }, 120);

        setTimeout(() => {
            wrap.classList.remove('show');
            galleryCaption.classList.remove('show');
            setTimeout(() => {
                wrap.remove();
                galleryStage.classList.remove('bad1-mode');
                galleryImg.style.display = '';
                onDone();
            }, 1200);
        }, 4500);
    }, 600);
}

function playSequence(list, index, onDone) {
    if (index >= list.length) { onDone(); return; }
    const item = list[index];
    galleryImg.classList.remove('show');
    galleryCaption.classList.remove('show');
    setTimeout(() => {
        galleryImg.src = item.src;
        galleryCaption.textContent = item.caption;
        galleryImg.onload = () => {
            galleryImg.classList.add('show');
            galleryCaption.classList.add('show');
            setTimeout(() => { playSequence(list, index + 1, onDone); }, 3200);
        };
        galleryImg.onerror = () => {
            galleryCaption.textContent = item.caption;
            galleryCaption.classList.add('show');
            setTimeout(() => { playSequence(list, index + 1, onDone); }, 2000);
        };
    }, 700);
}

function showTransition(onDone) {
    galleryStage.classList.add('hidden');
    const overlay = document.createElement('div');
    overlay.id = 'ending-overlay';
    overlay.innerHTML = `<div class="ending-line" id="gallery-line"></div>`;
    document.body.appendChild(overlay);
    const lineEl = document.getElementById('gallery-line');
    let i = 0;
    function nextLine() {
        if (i >= TRANSITION_LINES.length) {
            overlay.remove();
            galleryStage.classList.remove('hidden');
            onDone();
            return;
        }
        lineEl.textContent = TRANSITION_LINES[i];
        lineEl.style.opacity = '1';
        setTimeout(() => {
            lineEl.style.opacity = '0';
            setTimeout(() => { i++; nextLine(); }, 1200);
        }, 2500);
    }
    nextLine();
}

function onGalleryFinish() {
    startEnding();
}