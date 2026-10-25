// ============ 核心规则 ============
function getCardType(cards) {
  if (!cards || cards.length === 0) return null;
  const sorted = [...cards].sort((a, b) => a.weight - b.weight);
  const len = sorted.length;
  const weights = sorted.map(c => c.weight);
  const counts = {};
  weights.forEach(w => counts[w] = (counts[w] || 0) + 1);
  const uniqueWeights = Object.keys(counts).map(Number).sort((a, b) => a - b);

  if (len === 2 && weights.includes(16) && weights.includes(17)) return { type: 'rocket', weight: 100, length: 1 };
  if (len === 4 && uniqueWeights.length === 1) return { type: 'bomb', weight: uniqueWeights[0], length: 1 };
  if (len === 1) return { type: 'single', weight: uniqueWeights[0], length: 1 };
  if (len === 2 && uniqueWeights.length === 1) return { type: 'pair', weight: uniqueWeights[0], length: 1 };
  if (len === 3 && uniqueWeights.length === 1) return { type: 'triple', weight: uniqueWeights[0], length: 1 };
  
  if (len === 4 && uniqueWeights.length === 2) {
    let triple = uniqueWeights.find(w => counts[w] === 3);
    if (triple) return { type: 'triple_single', weight: triple, length: 1 };
  }
  if (len === 5 && uniqueWeights.length === 2) {
    let triple = uniqueWeights.find(w => counts[w] === 3);
    let pair = uniqueWeights.find(w => counts[w] === 2);
    if (triple && pair) return { type: 'triple_pair', weight: triple, length: 1 };
  }

  if (len >= 5 && uniqueWeights.length === len && uniqueWeights[len - 1] <= 14) {
    let isStraight = true;
    for (let i = 1; i < len; i++) {
      if (uniqueWeights[i] - uniqueWeights[i - 1] !== 1) { isStraight = false; break; }
    }
    if (isStraight) return { type: 'straight', weight: uniqueWeights[0], length: len };
  }

  if (len >= 6 && len % 2 === 0 && uniqueWeights.length === len / 2 && uniqueWeights[uniqueWeights.length - 1] <= 14) {
    let isPairs = true;
    for (let w of uniqueWeights) if (counts[w] !== 2) { isPairs = false; break; }
    if (isPairs) {
      let isConsecutive = true;
      for (let i = 1; i < uniqueWeights.length; i++) {
        if (uniqueWeights[i] - uniqueWeights[i - 1] !== 1) { isConsecutive = false; break; }
      }
      if (isConsecutive) return { type: 'pair_straight', weight: uniqueWeights[0], length: uniqueWeights.length };
    }
  }

  let triples = uniqueWeights.filter(w => counts[w] === 3 && w <= 14);
  if (triples.length >= 2) {
    let sequences = [];
    let currentSeq = [triples[0]];
    for (let i = 1; i < triples.length; i++) {
      if (triples[i] - triples[i - 1] === 1) currentSeq.push(triples[i]);
      else { sequences.push(currentSeq); currentSeq = [triples[i]]; }
    }
    sequences.push(currentSeq);

    for (let seq of sequences) {
      if (seq.length >= 2) {
        if (len === seq.length * 3 && uniqueWeights.length === seq.length) {
          return { type: 'plane', weight: seq[0], length: seq.length };
        }
        if (len === seq.length * 4) {
          return { type: 'plane_single', weight: seq[0], length: seq.length };
        }
        if (len === seq.length * 5) {
           let pairCount = 0;
           for (let w in counts) if (counts[w] === 2) pairCount++;
           if (pairCount === seq.length) {
             return { type: 'plane_pair', weight: seq[0], length: seq.length };
           }
        }
      }
    }
  }
  return null;
}

function canBeat(A, B) {
  if (!B) return true;
  if (A.type === 'rocket') return true;
  if (B.type === 'rocket') return false;
  if (A.type === 'bomb' && B.type !== 'bomb') return true; // 炸弹可以压任何非炸弹
  if (A.type !== 'bomb' && B.type === 'bomb') return false; // 非炸弹不能压炸弹
  if (A.type === B.type && A.length === B.length) {
    return A.weight > B.weight;
  }
  return false;
}

// ============ 游戏主逻辑 ============
(function() {
  const SUITS = ['♠', '♥', '♣', '♦'];
  const RANKS = ['3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A', '2', '小王', '大王'];
  const RANK_WEIGHT = {
    '3': 3, '4': 4, '5': 5, '6': 6, '7': 7, '8': 8, '9': 9, '10': 10,
    'J': 11, 'Q': 12, 'K': 13, 'A': 14, '2': 15, '小王': 16, '大王': 17
  };

  const bottomCardsEl = document.getElementById('bottomCards');
  const playCardsEl = document.getElementById('playCards');
  const leftPlayAreaEl = document.getElementById('leftPlayArea');
  const rightPlayAreaEl = document.getElementById('rightPlayArea');
  const handAreaEl = document.getElementById('handArea');
  const roleDisplay = document.getElementById('roleDisplay');
  const countDisplay = document.getElementById('countDisplay');
  const aiCountDisplay = document.getElementById('aiCountDisplay');
  const msgBox = document.getElementById('msgBox');
  
  const callBtn = document.getElementById('callBtn');
  const noCallBtn = document.getElementById('noCallBtn');
  const passBtn = document.getElementById('passBtn');
  const playBtn = document.getElementById('playBtn');
  const hintBtn = document.getElementById('hintBtn');
  const levelBtn = document.getElementById('levelBtn');
  const resetBtn = document.getElementById('resetBtn');
  
  const levelOverlay = document.getElementById('levelOverlay');
  const levelClose = document.getElementById('levelClose');
  
  let aiDifficulty = 'normal';
  let playerHand = [], leftAiHand = [], rightAiHand = [], bottomHand = [];
  let selectedCards = [];
  let isLandlord = false;
  let currentTurn = 0;
  let lastPlay = null;
  let passCount = 0;
  let gameState = 'calling';

  function renderCard(card, isSmall = false) {
    const el = document.createElement('div');
    el.className = `card ${isSmall ? 'small' : ''} ${card.suit === '♥' || card.suit === '♦' ? 'red' : 'black'}`;
    el.innerHTML = `<div>${card.rank}</div><div class="suit">${card.suit !== 'JOKER' ? card.suit : ''}</div>`;
    return el;
  }

  function renderHand() {
    handAreaEl.innerHTML = '';
    playerHand.forEach(card => {
      const isSelected = selectedCards.includes(card);
      const el = renderCard(card);
      if (isSelected) el.classList.add('selected');
      el.style.marginLeft = isSelected ? '-15px' : '-28px';
      if (playerHand.indexOf(card) === 0) el.style.marginLeft = '0';

      const handler = (e) => {
        e.preventDefault();
        if (gameState !== 'playing' || currentTurn !== 0) return;
        const idx = selectedCards.indexOf(card);
        if (idx > -1) selectedCards.splice(idx, 1);
        else selectedCards.push(card);
        renderHand();
      };
      el.addEventListener('click', handler);
      el.addEventListener('touchstart', handler, { passive: false });
      handAreaEl.appendChild(el);
    });
    countDisplay.textContent = playerHand.length;
    aiCountDisplay.textContent = `${leftAiHand.length} / ${rightAiHand.length}`;
  }

  function renderBottom(isOpen) {
    bottomCardsEl.innerHTML = '';
    bottomHand.forEach(card => {
      const el = renderCard(card, true);
      if (!isOpen) el.className = 'card small back';
      bottomCardsEl.appendChild(el);
    });
  }

  function showMsg(text, isErr = true) {
    msgBox.textContent = text;
    msgBox.style.color = isErr ? '#ff6f61' : '#7ee0a0';
    msgBox.classList.add('show');
    setTimeout(() => msgBox.classList.remove('show'), 1500);
  }

  function updateControls() {
    const myTurn = currentTurn === 0 && gameState === 'playing';
    callBtn.style.display = gameState === 'calling' ? 'block' : 'none';
    noCallBtn.style.display = gameState === 'calling' ? 'block' : 'none';
    playBtn.disabled = !myTurn;
    passBtn.disabled = !myTurn || !lastPlay || lastPlay.player === 0;
    hintBtn.disabled = !myTurn;
  }

  function initDeck() {
    let deck = [];
    for (let rank of RANKS) {
      if (rank === '小王' || rank === '大王') continue;
      for (let suit of SUITS) deck.push({ rank, suit, weight: RANK_WEIGHT[rank] });
    }
    deck.push({ rank: '小王', suit: 'JOKER', weight: 16 });
    deck.push({ rank: '大王', suit: 'JOKER', weight: 17 });
    for (let i = deck.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [deck[i], deck[j]] = [deck[j], deck[i]];
    }
    return deck;
  }

  function startGame() {
    const deck = initDeck();
    playerHand = deck.slice(0, 17).sort((a, b) => b.weight - a.weight);
    leftAiHand = deck.slice(17, 34).sort((a, b) => b.weight - a.weight);
    rightAiHand = deck.slice(34, 51).sort((a, b) => b.weight - a.weight);
    bottomHand = deck.slice(51, 54).sort((a, b) => b.weight - a.weight);
    
    selectedCards = [];
    clearAllPlayAreas();
    isLandlord = false;
    gameState = 'calling';
    currentTurn = 0;
    lastPlay = null;
    passCount = 0;
    
    renderBottom(false);
    renderHand();
    updateControls();
    roleDisplay.textContent = '未知';
    showMsg('请决定是否叫地主', false);
  }

  function clearAllPlayAreas() {
    playCardsEl.innerHTML = '';
    leftPlayAreaEl.innerHTML = '';
    rightPlayAreaEl.innerHTML = '';
  }

  callBtn.addEventListener('click', () => {
    isLandlord = true;
    gameState = 'playing';
    playerHand = playerHand.concat(bottomHand).sort((a, b) => b.weight - a.weight);
    renderBottom(true);
    renderHand();
    roleDisplay.textContent = '地主';
    showMsg('你是地主，请出牌', false);
    currentTurn = 0;
    updateControls();
  });

  noCallBtn.addEventListener('click', () => {
    isLandlord = false;
    gameState = 'playing';
    leftAiHand = leftAiHand.concat(bottomHand).sort((a, b) => b.weight - a.weight);
    renderBottom(true);
    renderHand();
    roleDisplay.textContent = '农民';
    showMsg('左边电脑成为地主，轮到你出牌', false);
    currentTurn = 0;
    updateControls();
  });

  playBtn.addEventListener('click', () => {
    if (selectedCards.length === 0) return;
    const typeInfo = getCardType(selectedCards);
    if (!typeInfo) return showMsg('牌型不合法');
    if (!canBeat(typeInfo, lastPlay)) return showMsg('大不过上家，要不起');

    playCardsEl.innerHTML = '';
    [...selectedCards].sort((a, b) => b.weight - a.weight).forEach(c => playCardsEl.appendChild(renderCard(c, true)));

    playerHand = playerHand.filter(c => !selectedCards.includes(c));
    selectedCards = [];
    lastPlay = { ...typeInfo, player: 0 };
    passCount = 0;
    renderHand();
    
    if (playerHand.length === 0) return endGame(true);
    nextTurn();
  });

  passBtn.addEventListener('click', () => {
    if (!lastPlay || lastPlay.player === 0) return;
    selectedCards = [];
    renderHand();
    showMsg('不出', false);
    passCount++;
    
    if (passCount >= 2) {
      lastPlay = null;
      clearAllPlayAreas();
      passCount = 0;
    }
    nextTurn();
  });

  function nextTurn() {
    currentTurn = (currentTurn + 1) % 3;
    updateControls();
    if (gameState === 'playing' && currentTurn !== 0) {
      setTimeout(aiTurn, 1000);
    } else if (currentTurn === 0) {
      showMsg('轮到你出牌', false);
    }
  }

  function aiTurn() {
    if (gameState !== 'playing') return;
    const aiHand = currentTurn === 1 ? leftAiHand : rightAiHand;
    const playArea = currentTurn === 1 ? leftPlayAreaEl : rightPlayAreaEl;
    
    const aiRole = (currentTurn === 1 && !isLandlord) || (currentTurn === 2 && isLandlord) ? 'landlord' : 'farmer';
    const playToMake = getAIMove(aiHand, lastPlay, aiDifficulty, aiRole, lastPlay ? lastPlay.player : -1);

    if (playToMake) {
      playArea.innerHTML = '';
      [...playToMake.cards].sort((a, b) => b.weight - a.weight).forEach(c => playArea.appendChild(renderCard(c, true)));
      
      if (currentTurn === 1) leftAiHand = leftAiHand.filter(c => !playToMake.cards.includes(c));
      else rightAiHand = rightAiHand.filter(c => !playToMake.cards.includes(c));
      
      lastPlay = { ...playToMake, player: currentTurn };
      passCount = 0;
      renderHand();

      if ((currentTurn === 1 && leftAiHand.length === 0) || (currentTurn === 2 && rightAiHand.length === 0)) {
        return endGame(aiRole === 'farmer'); // 农民胜利（队友赢就是自己赢）
      }
    } else {
      showMsg('不出', false);
      playArea.innerHTML = '';
      passCount++;
      if (passCount >= 2) {
        lastPlay = null;
        clearAllPlayAreas();
        passCount = 0;
      }
    }
    nextTurn();
  }

  hintBtn.addEventListener('click', () => {
    if (currentTurn !== 0) return;
    const plays = findAllPlays(playerHand);
    let hintPlay = null;
    if (!lastPlay) {
      hintPlay = plays.find(p => p.type === 'single');
    } else {
      hintPlay = plays.find(p => canBeat(p, lastPlay));
    }
    if (hintPlay) {
      selectedCards = [...hintPlay.cards];
      renderHand();
    } else {
      showMsg('没有能大过的牌', true);
    }
  });

  function endGame(isWin) {
    gameState = 'over';
    if (isWin) showMsg('大获全胜！', false);
    else showMsg('你输了！', true);
    updateControls();
  }

  // 难度逻辑
  function showLevelSelect() {
    levelOverlay.querySelectorAll('.level-btn[data-level]').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.level === aiDifficulty);
    });
    levelOverlay.classList.remove('hidden');
  }

  levelBtn.addEventListener('click', showLevelSelect);
  levelClose.addEventListener('click', () => levelOverlay.classList.add('hidden'));
  
  levelOverlay.querySelectorAll('.level-btn[data-level]').forEach(btn => {
    btn.addEventListener('click', () => {
      aiDifficulty = btn.dataset.level;
      levelOverlay.classList.add('hidden');
      showMsg(`难度已设为: ${aiDifficulty}`, false);
    });
  });

  resetBtn.addEventListener('click', startGame);
  startGame();
})();