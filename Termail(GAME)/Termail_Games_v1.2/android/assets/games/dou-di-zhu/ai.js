// 基础牌型判定（保持不变，省略... 复制上一版的 getCardType 和 canBeat 即可）

function findAllPlays(hand) {
  // ... (保持不变，复制上一版的)
}

// AI 核心出牌逻辑
// role: 'landlord' 或 'farmer'
// isTeammateWinning: 队友上一手牌是不是快赢了（剩 1-2 张）
function getAIMove(hand, lastPlay, difficulty, role, lastPlayPlayer) {
  const possible = findAllPlays(hand);
  let valid = lastPlay ? possible.filter(p => canBeat(p, lastPlay)) : possible;

  if (valid.length === 0) return null;

  // 如果是困难模式，且自己是农民，且上家是队友，且队友牌很少了 -> 不要压队友
  if (difficulty === 'hard' && role === 'farmer') {
    // 假设 1 是左边 AI，2 是右边 AI。如果上家是另一个农民
    // 简单处理：在游戏主逻辑里通过 lastPlayPlayer 判断是否是队友
    // 这里简单写：如果困难 AI 发现队友只剩几张牌，且队友出的牌不小，就放水
    // (具体配合逻辑需要主程序把队友手牌数量传进来，这里简化为遇到队友出牌不压)
    if (lastPlayPlayer === (role === 'farmer' ? 1 : 2) || lastPlayPlayer === (role === 'farmer' ? 2 : 1)) {
       // 这里稍微简化，实际应该判断 lastPlayPlayer 和 当前 AI 是不是同一阵营
    }
  }

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

  // 困难 AI：优先走最长组合，压牌时用最小的
  if (difficulty === 'hard') {
    if (!lastPlay) {
      valid.sort((a, b) => b.cards.length - a.cards.length || b.weight - a.weight);
      return valid[0];
    } else {
      valid.sort((a, b) => a.weight - b.weight);
      // 如果是地主，且农民出了大牌，直接上炸弹
      if (role === 'landlord' && lastPlay.weight >= 14) {
        const bomb = valid.find(p => p.type === 'bomb' || p.type === 'rocket');
        if (bomb) return bomb;
      }
      return valid[0];
    }
  }

  return valid[0];
}