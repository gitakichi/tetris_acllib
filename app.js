const COLS = 10;
const ROWS = 20;
const BLOCK = 21;
const EMPTY = 0;

const COLORS = {
  0: '#07111f',
  1: '#38bdf8',
  2: '#a78bfa',
  3: '#fb7185',
  4: '#fbbf24',
  5: '#4ade80',
  6: '#fb923c',
  7: '#f472b6'
};

const SHAPES = [
  { name: 'I', color: 1, matrix: [[0, 0, 0, 0], [1, 1, 1, 1], [0, 0, 0, 0], [0, 0, 0, 0]] },
  { name: 'J', color: 2, matrix: [[1, 0, 0], [1, 1, 1], [0, 0, 0]] },
  { name: 'L', color: 3, matrix: [[0, 0, 1], [1, 1, 1], [0, 0, 0]] },
  { name: 'O', color: 4, matrix: [[1, 1], [1, 1]] },
  { name: 'S', color: 5, matrix: [[0, 1, 1], [1, 1, 0], [0, 0, 0]] },
  { name: 'T', color: 6, matrix: [[0, 1, 0], [1, 1, 1], [0, 0, 0]] },
  { name: 'Z', color: 7, matrix: [[1, 1, 0], [0, 1, 1], [0, 0, 0]] }
];

const boardCanvas = document.getElementById('board');
const nextCanvas = document.getElementById('next');
const next2Canvas = document.getElementById('next2');
const holdCanvas = document.getElementById('hold');
const scoreEl = document.getElementById('score');
const statusEl = document.getElementById('statusText');
const restartBtn = document.getElementById('restartBtn');

const boardCtx = boardCanvas.getContext('2d');
const nextCtx = nextCanvas.getContext('2d');
const next2Ctx = next2Canvas.getContext('2d');
const holdCtx = holdCanvas.getContext('2d');

let board = createMatrix();
let currentPiece = null;
let queue = [];
let holdPiece = null;
let targetEnabled = true;
let lineClearRows = [];
let lineClearBlink = false;
let lineClearTimer = null;
let dropInterval = 700;
let score = 0;
let gameOver = false;
let paused = false;
let timer = null;

function createMatrix() {
  return Array.from({ length: ROWS }, () => Array(COLS).fill(EMPTY));
}

function randomPiece() {
  const type = SHAPES[Math.floor(Math.random() * SHAPES.length)];
  return {
    name: type.name,
    color: type.color,
    matrix: type.matrix.map(row => [...row])
  };
}

function clonePiece(piece) {
  return {
    name: piece.name,
    color: piece.color,
    matrix: piece.matrix.map(row => [...row])
  };
}

function ensureQueue() {
  while (queue.length < 3) queue.push(randomPiece());
}

function nextFromQueue() {
  ensureQueue();
  const piece = queue.shift();
  queue.push(randomPiece());
  return clonePiece(piece);
}

function spawnPiece() {
  currentPiece = nextFromQueue();
  const startX = Math.floor((COLS - currentPiece.matrix[0].length) / 2);
  const startY = 0;

  if (!isValidMove(currentPiece, startX, startY, board)) {
    gameOver = true;
    updateStatus('Game Over — Press R to restart');
    stopLoop();
  }

  return { x: startX, y: startY };
}

let piece = { x: 0, y: 0 };

function isValidMove(pieceData, x, y, targetBoard) {
  for (let row = 0; row < pieceData.matrix.length; row++) {
    for (let col = 0; col < pieceData.matrix[row].length; col++) {
      if (!pieceData.matrix[row][col]) continue;
      const newX = x + col;
      const newY = y + row;
      if (newX < 0 || newX >= COLS || newY >= ROWS) return false;
      if (newY >= 0 && targetBoard[newY][newX]) return false;
    }
  }
  return true;
}

function rotateMatrix(matrix) {
  return matrix[0].map((_, i) => matrix.map(row => row[i]).reverse());
}

function rotatePiece() {
  if (!currentPiece) return;
  const rotated = rotateMatrix(currentPiece.matrix);
  const backup = currentPiece.matrix;
  currentPiece.matrix = rotated;
  if (!isValidMove(currentPiece, piece.x, piece.y, board)) {
    currentPiece.matrix = backup;
  }
}

function mergePiece() {
  currentPiece.matrix.forEach((row, r) => {
    row.forEach((value, c) => {
      if (value) {
        const y = piece.y + r;
        const x = piece.x + c;
        if (y >= 0) board[y][x] = currentPiece.color;
      }
    });
  });
}

function findFullRows() {
  return board
    .map((row, index) => (row.every(cell => cell !== EMPTY) ? index : -1))
    .filter(index => index >= 0);
}

function clearLines(rows) {
  if (!rows.length) return 0;

  const cleared = [...rows];
  for (let r = ROWS - 1; r >= 0; r--) {
    if (cleared.includes(r)) {
      board.splice(r, 1);
      board.unshift(Array(COLS).fill(EMPTY));
    }
  }

  const linePoints = [0, 100, 300, 500, 800];
  score += linePoints[cleared.length] || 0;
  updateScore();
  return cleared.length;
}

function hardDrop() {
  while (isValidMove(currentPiece, piece.x, piece.y + 1, board)) {
    piece.y += 1;
  }
  lockPiece();
}

function holdCurrentPiece() {
  if (!currentPiece || gameOver || paused) return;

  if (!holdPiece) {
    holdPiece = clonePiece(currentPiece);
    piece = spawnPiece();
  } else {
    const swapped = clonePiece(currentPiece);
    currentPiece = clonePiece(holdPiece);
    holdPiece = swapped;
    piece = {
      x: Math.floor((COLS - currentPiece.matrix[0].length) / 2),
      y: 0
    };
    if (!isValidMove(currentPiece, piece.x, piece.y, board)) {
      gameOver = true;
      updateStatus('Game Over — Press R to restart');
      stopLoop();
      return;
    }
  }

  draw();
}

function finalizeLock() {
  piece = spawnPiece();
  if (gameOver) return;
  updateStatus('Playing');
  draw();
}

function startLineClearEffect(rows) {
  lineClearRows = rows;
  lineClearBlink = true;
  updateStatus('Line clear!');
  draw();

  if (lineClearTimer) clearInterval(lineClearTimer);
  lineClearTimer = window.setInterval(() => {
    lineClearBlink = !lineClearBlink;
    draw();
  }, 160);

  window.setTimeout(() => {
    if (lineClearTimer) clearInterval(lineClearTimer);
    lineClearTimer = null;
    clearLines(rows);
    lineClearRows = [];
    finalizeLock();
  }, 320);
}

function lockPiece() {
  mergePiece();
  const rows = findFullRows();
  if (rows.length > 0) {
    startLineClearEffect(rows);
    return;
  }
  finalizeLock();
}

function movePiece(dx, dy) {
  if (!currentPiece || gameOver || paused) return;
  const nextX = piece.x + dx;
  const nextY = piece.y + dy;

  if (isValidMove(currentPiece, nextX, nextY, board)) {
    piece.x = nextX;
    piece.y = nextY;
    draw();
    return true;
  }

  if (dy === 1) {
    lockPiece();
  }
  return false;
}

function startLoop() {
  stopLoop();
  timer = window.setInterval(() => {
    if (!paused && !gameOver) movePiece(0, 1);
  }, dropInterval);
}

function stopLoop() {
  if (timer) clearInterval(timer);
}

function updateScore() {
  scoreEl.textContent = String(score);
}

function updateStatus(text) {
  statusEl.textContent = text;
}

function drawCell(ctx, x, y, color, outline = true) {
  const px = x * BLOCK;
  const py = y * BLOCK;
  ctx.fillStyle = color;
  ctx.fillRect(px + 1, py + 1, BLOCK - 2, BLOCK - 2);
  if (outline) {
    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.strokeRect(px + 1, py + 1, BLOCK - 2, BLOCK - 2);
  }
}

function getGhostY() {
  if (!currentPiece) return piece.y;
  let ghostY = piece.y;
  while (isValidMove(currentPiece, piece.x, ghostY + 1, board)) {
    ghostY += 1;
  }
  return ghostY;
}

function drawBoard() {
  boardCtx.clearRect(0, 0, boardCanvas.width, boardCanvas.height);
  for (let y = 0; y < ROWS; y++) {
    for (let x = 0; x < COLS; x++) {
      const isClearing = lineClearRows.includes(y) && lineClearBlink;
      const color = isClearing ? COLORS[0] : COLORS[board[y][x]] || COLORS[0];
      drawCell(boardCtx, x, y, color);
    }
  }

  if (currentPiece && targetEnabled) {
    const ghostY = getGhostY();
    currentPiece.matrix.forEach((row, r) => {
      row.forEach((value, c) => {
        if (value) drawCell(boardCtx, piece.x + c, ghostY + r, 'rgba(148, 163, 184, 0.28)', false);
      });
    });
  }

  if (currentPiece) {
    currentPiece.matrix.forEach((row, r) => {
      row.forEach((value, c) => {
        const y = piece.y + r;
        const x = piece.x + c;
        if (!value) return;
        if (lineClearRows.includes(y)) {
          if (lineClearBlink) return;
        }
        drawCell(boardCtx, x, y, COLORS[currentPiece.color], true);
      });
    });
  }
}

function drawPreview(ctx, canvas, piece) {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (!piece) return;
  const matrix = piece.matrix;
  const offsetX = Math.floor((4 - matrix[0].length) / 2);
  const offsetY = Math.floor((4 - matrix.length) / 2);
  matrix.forEach((row, r) => {
    row.forEach((value, c) => {
      if (value) drawCell(ctx, offsetX + c, offsetY + r, COLORS[piece.color], true);
    });
  });
}

function drawNext() {
  const preview1 = queue[0] || randomPiece();
  const preview2 = queue[1] || randomPiece();
  drawPreview(nextCtx, nextCanvas, preview1);
  drawPreview(next2Ctx, next2Canvas, preview2);
  drawPreview(holdCtx, holdCanvas, holdPiece);
}

function draw() {
  drawBoard();
  drawNext();
}

function resetGame() {
  board = createMatrix();
  score = 0;
  gameOver = false;
  paused = false;
  holdPiece = null;
  queue = [];
  updateScore();
  updateStatus('Playing');
  ensureQueue();
  piece = spawnPiece();
  draw();
  startLoop();
}

window.addEventListener('keydown', (event) => {
  const isArrowKey = ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown'].includes(event.key);
  const isSpace = event.key === ' ';

  if (isArrowKey || isSpace) {
    event.preventDefault();
  }

  if (event.key === 'p' || event.key === 'P') {
    paused = !paused;
    updateStatus(paused ? 'Paused — Press P to resume' : 'Playing');
    return;
  }

  if (event.key === 'r' || event.key === 'R') {
    resetGame();
    return;
  }

  if (gameOver || paused) return;

  switch (event.key) {
    case 'f':
    case 'F':
      targetEnabled = !targetEnabled;
      updateStatus(targetEnabled ? 'Target preview: ON' : 'Target preview: OFF');
      draw();
      break;
    case 'h':
    case 'H':
      holdCurrentPiece();
      break;
    case 'ArrowLeft':
      movePiece(-1, 0);
      break;
    case 'ArrowRight':
      movePiece(1, 0);
      break;
    case 'ArrowDown':
      movePiece(0, 1);
      break;
    case 'ArrowUp':
      rotatePiece();
      draw();
      break;
    case ' ':
      hardDrop();
      draw();
      break;
  }
});

restartBtn.addEventListener('click', resetGame);

resetGame();
