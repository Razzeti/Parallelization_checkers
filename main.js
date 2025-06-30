const boardDiv = document.getElementById('board');
const statusSpan = document.getElementById('status');
const statThreadsSpan = document.getElementById('stat-threads');
const statTimeSpan = document.getElementById('stat-time');

let gamePtr = null;
let fromSquare = null;

// API de C++
let crearJuego, getTablero, realizarMovimiento, getMovimientoIA, getLastAITime, getThreadCount, getMovimientosLegalesStr;

Module.onRuntimeInitialized = () => {
    crearJuego = Module.cwrap('crear_juego', 'number', []);
    getTablero = Module.cwrap('get_tablero', 'string', ['number']);
    realizarMovimiento = Module.cwrap('realizar_movimiento', 'boolean', ['number', 'string']);
    getMovimientoIA = Module.cwrap('get_movimiento_ia', 'string', ['number']);
    getLastAITime = Module.cwrap('get_last_ai_time', 'number', ['number']);
    getThreadCount = Module.cwrap('get_thread_count', 'number', ['number']);
    getMovimientosLegalesStr = Module.cwrap('get_movimientos_legales_str', 'string', ['number']);

    gamePtr = crearJuego();
    statusSpan.textContent = 'Turno de Blancas (B).';
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
            const pieceDiv = document.createElement('div');
            pieceDiv.classList.add('pieza');
            switch (piece) {
                case 'B': pieceDiv.classList.add('blanca'); break;
                case 'N': pieceDiv.classList.add('negra'); break;
                case 'X': pieceDiv.classList.add('reina-blanca'); break;
                case 'Y': pieceDiv.classList.add('reina-negra'); break;
            }
            square.appendChild(pieceDiv);
        }
        
        if ((row + col) % 2 === 1) {
            square.addEventListener('click', () => onSquareClick(row, col));
        }
        boardDiv.appendChild(square);
    }
}

function onSquareClick(row, col) {
    if (fromSquare === null) {
        const boardString = getTablero(gamePtr);
        const piece = boardString[row * 10 + col];
        if (piece === 'B' || piece === 'X') { 
            fromSquare = { row, col };
            document.querySelectorAll('.selected').forEach(el => el.classList.remove('selected'));
            document.querySelector(`[data-row='${row}'][data-col='${col}']`).classList.add('selected');
        }
    } else {
        const fromColChar = String.fromCharCode('A'.charCodeAt(0) + fromSquare.col);
        const fromRowChar = 10 - fromSquare.row;
        const toColChar = String.fromCharCode('A'.charCodeAt(0) + col);
        const toRowChar = 10 - row;
        const moveStr = `${fromColChar}${fromRowChar} ${toColChar}${toRowChar}`;

        const legalMovesStr = getMovimientosLegalesStr(gamePtr);
        const legalMoves = legalMovesStr.split('|');

        if (legalMoves.includes(moveStr)) {
            realizarMovimiento(gamePtr, moveStr);
            renderizarTablero();
            statusSpan.textContent = 'IA está pensando...';
            setTimeout(triggerAIMove, 50);
        } else {
            statusSpan.textContent = 'Movimiento ilegal. Inténtalo de nuevo.';
            document.querySelectorAll('.selected').forEach(el => el.classList.remove('selected'));
        }
        
        fromSquare = null;
    }
}

function triggerAIMove() {
    const aiMove = getMovimientoIA(gamePtr);
    if (aiMove === "") {
        statusSpan.textContent = '¡GANASTE! La IA no tiene movimientos.';
        return;
    }

    const aiTime = getLastAITime(gamePtr);
    const threadCount = getThreadCount(gamePtr);
    statTimeSpan.textContent = `${aiTime.toFixed(2)} ms`;
    statThreadsSpan.textContent = threadCount;
    
    realizarMovimiento(gamePtr, aiMove);
    statusSpan.textContent = `Turno de Blancas (B).`;
    renderizarTablero();
}
