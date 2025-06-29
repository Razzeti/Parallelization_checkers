const boardDiv = document.getElementById('board');
const statusDiv = document.getElementById('status');
const aiMoveBtn = document.getElementById('aiMoveBtn');
let gamePtr = null;
let fromSquare = null;

let crearJuego, getTablero, realizarMovimiento, getMovimientoIA;

Module.onRuntimeInitialized = () => {
    crearJuego = Module.cwrap('crear_juego', 'number', []);
    getTablero = Module.cwrap('get_tablero', 'string', ['number']);
    realizarMovimiento = Module.cwrap('realizar_movimiento', 'boolean', ['number', 'string']);
    getMovimientoIA = Module.cwrap('get_movimiento_ia', 'string', ['number']);

    gamePtr = crearJuego();
    statusDiv.textContent = 'Turno de Blancas (B). Haz tu movimiento.';
    aiMoveBtn.disabled = true;
    renderizarTablero();
};

function renderizarTablero() {
    if (!gamePtr) return;
    const boardString = getTablero(gamePtr);
    boardDiv.innerHTML = '';

    for (let i = 0; i < 100; i++) {
        const square = document.createElement('div');
        const row = Math.floor(i / 10);
        const col = i % 10;
        square.classList.add('square');
        square.classList.add((row + col) % 2 === 1 ? 'dark' : 'light');
        square.dataset.row = row;
        square.dataset.col = col;

        const piece = boardString[i];
        if (piece !== '.') {
            let pieceSymbol = '';
            switch (piece) {
                case 'B': pieceSymbol = '⚪'; break;
                case 'N': pieceSymbol = '⚫'; break;
                case 'X': pieceSymbol = '👑'; break;
                case 'Y': pieceSymbol = '♛'; break;
            }
            square.textContent = pieceSymbol;
        }
        
        if ((row + col) % 2 === 1) {
            square.addEventListener('click', () => onSquareClick(row, col));
        }
        boardDiv.appendChild(square);
    }
}

function onSquareClick(row, col) {
    if (fromSquare === null) {
        fromSquare = { row, col };
        document.querySelectorAll('.selected').forEach(el => el.classList.remove('selected'));
        document.querySelector(`[data-row='${row}'][data-col='${col}']`).classList.add('selected');
    } else {
        const fromColChar = String.fromCharCode('A'.charCodeAt(0) + fromSquare.col);
        const fromRowChar = 10 - fromSquare.row;
        const toColChar = String.fromCharCode('A'.charCodeAt(0) + col);
        const toRowChar = 10 - row;
        const moveStr = `${fromColChar}${fromRowChar} ${toColChar}${toRowChar}`;
        
        realizarMovimiento(gamePtr, moveStr);
        
        fromSquare = null;
        renderizarTablero();
        
        statusDiv.textContent = 'Turno de Negras (IA)...';
        aiMoveBtn.disabled = false;
    }
}

aiMoveBtn.addEventListener('click', () => {
    aiMoveBtn.disabled = true;
    statusDiv.textContent = 'IA está pensando...';
    
    setTimeout(() => {
        const aiMove = getMovimientoIA(gamePtr);
        realizarMovimiento(gamePtr, aiMove);
        statusDiv.textContent = `La IA movió ${aiMove}. Turno de Blancas (B).`;
        renderizarTablero();
    }, 50);
})
