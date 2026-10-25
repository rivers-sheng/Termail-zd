function findAllPlays(hand) {
  if (!hand || hand.length === 0) return [];
  const plays = [];
  const counts = {};
  hand.forEach(c => counts[c.weight] = (counts[c.weight] || 0) + 1);

  for (let w in counts) {
    plays.push({ type: 'single', weight: Number(w), length: 1, cards: [hand.find(c => c.weight === Number(w))] });
  }
  for (let w in counts) if (counts[w] >= 2) {
    plays.push({ type: 'pair', weight: Number(w), length: 1, cards: hand.filter(c => c.weight === Number(w)).slice(0, 2) });
  }
  for (let w in counts) if (counts[w] >= 3) {
    plays.push({ type: 'triple', weight: Number(w), length: 1, cards: hand.filter(c => c.weight === Number(w)).slice(0, 3) });
  }
  for (let w in counts) if (counts[w] >= 4) {
    plays.push({ type: 'bomb', weight: Number(w), length: 1, cards: hand.filter(c => c.weight === Number(w)) });
  }
  if (counts[16] && counts[17]) {
    plays.push({ type: 'rocket', weight: 100, length: 1, cards: hand.filter(c => c.weight === 16 || c.weight === 17) });
  }

  // 简单的顺子检测（长度5）
  const uniqueW = Object.keys(counts).map(Number).sort((a,b)=>a-b);
  for (let i = 0; i <= uniqueW.length - 5; i++) {
    let seq = uniqueW.slice(i, i + 5);
    let isSeq = seq.every((val, idx) => idx === 0 || val - seq[idx - 1] === 1);
    if (isSeq && seq[4] <= 14) {
      plays.push({ type: 'straight', weight: seq[0], length: 5, cards: seq.map(w => hand.find(c => c.weight === w)) });
    }
  }

  return plays;
}

function getAIMove(hand, lastPlay, difficulty, role, lastPlayPlayer) {
  const possible = findAllPlays(hand);
  let valid = lastPlay ? possible.filter(p => canBeat(p, lastPlay)) : possible;

  if (valid.length === 0) return null;

  // 简单 AI：先出单/对/三，不轻易出炸弹
  if (difficulty === 'easy') {
    const noBomb = valid.filter(p => p.type !== 'bomb' && p.type !== 'rocket');
    if (noBomb.length > 0) return noBomb[0];
    return valid[0];
  }

  // 普通 AI：会留炸弹，尽量用刚好压过的牌
  if (difficulty === 'normal') {
    valid.sort((a, b) => a.weight - b.weight);
    if (lastPlay) {
      const sameType = valid.find(p => p.type === lastPlay.type && p.length === lastPlay.length);
      if (sameType) return sameType;
      const bomb = valid.find(p => p.type === 'bomb' || p.type === 'rocket');
      if (bomb && lastPlay.type !== 'bomb') return bomb;
    }
    return valid[0];
  }

  // 困难 AI
  if (difficulty === 'hard') {
    if (!lastPlay) {
      valid.sort((a, b) => b.cards.length - a.cards.length || b.weight - a.weight);
      return valid[0];
    } else {
      valid.sort((a, b) => a.weight - b.weight);
      // 地主遇到大牌上炸弹
      if (role === 'landlord' && lastPlay.weight >= 14) {
        const bomb = valid.find(p => p.type === 'bomb' || p.type === 'rocket');
        if (bomb) return bomb;
      }
      // 农民之间配合（不压队友）
      if (role === 'farmer' && lastPlayPlayer !== 0 && lastPlay.weight < 14) {
        // 如果队友出的牌，且不是特别大，尽量别压
        return null; 
      }
      return valid[0];
    }
  }
  return valid[0];
}