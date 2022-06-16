DATASEG
    ; We keep these to track whether we can still castle
    whiteKingMoved db 0
    rightWhiteRookMoved db 0
    leftWhiteRookMoved db 0

    blackKingMoved db 0
    rightBlackRookMoved db 0
    leftBlackRookMoved db 0

    ; We'll need to keep a copy of the board's state in order to check if the king is in check without executing the move in the main board
    nextBoard db 64 dup (0)

CODESEG

proc checkSourcePick
    @@boardX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@boardY equ [word ptr bp + 4] ; In relation to the board, not the screen!
    push bp
    mov bp, sp
    push ax

    push @@boardX
    push @@boardY
    call getPieceValue ; Puts the value into AL
    cmp al, 0
    je @@isInvalid ; We'll check we haven't accidentally picked an empty square

    cmp [playerTurn], 0
    jg @@isWhiteTurn
    jl @@isBlackTurn

    @@isWhiteTurn:
        cmp al, 0 ; Checks whether we picked a black piece or a white piece, and goes to isValid/isInvalid accordingly
        jl @@isInvalid
            
        jmp @@isValid
    @@isBlackTurn:
        cmp al, 0 ; Checks whether we picked a black piece or a white piece, and goes to isValid/isInvalid accordingly
        jg @@isInvalid

        jmp @@isValid

    @@isInvalid:
        stc ; Turns the CARRY flag on so we can (JC label) later
        jmp @@sof
    @@isValid:
        clc ; Turns the CARRY flag off so we can (JNC label) later
    @@sof:

    pop ax
    pop bp
    ret 4
endp checkSourcePick
    
proc checkDestinationPick
    @@boardX equ [word ptr bp + 6]
    @@boardY equ [word ptr bp + 4]
    push bp
    mov bp, sp
    push ax

    push @@boardX
    push @@boardY
    call getPieceValue

    cmp [playerTurn], 0
    jg @@isWhiteTurn
    jl @@isBlackTurn

    @@isWhiteTurn:
        cmp al, 0  ; Checks whether we tried to go to a black piece or a white piece, and goes to isValid/isInvalid accordingly
        jg @@isInvalid

        jmp @@isValid
    @@isBlackTurn:
        cmp al, 0  ; Checks whether we tried to go to a black piece or a white piece, and goes to isValid/isInvalid accordingly
        jl @@isInvalid

        jmp @@isValid

    @@isInvalid:
        stc ; Turns the CARRY flag on so we can (JC label) later
        jmp @@sof
    @@isValid:
        clc ; Turns the CARRY flag off so we can (JNC label) later
    @@sof:
    
    pop ax
    pop bp
    ret 4
endp checkDestinationPick

proc validateMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx
    
    push @@sourceX
    push @@sourceY
    call getPieceValue ; AL now contains the piece value

    cbw ; This gets the absolute value in AL
    xor al, ah
    sub al, ah

    ; This is here just so I won't have to paste it to each piece option
    push @@sourceX
    push @@sourceY
    push @@destinationX
    push @@destinationY

    cmp al, 2
    je @@Pawn
    cmp al, 4
    je @@Bishop
    cmp al, 6
    je @@Knight
    cmp al, 8
    je @@Rook
    cmp al, 10
    je @@Queen
    cmp al, 12
    je @@King
    jmp @@sof

    @@Pawn:
        call validatePawnMove
        jmp @@sof
    @@Bishop:
        call validateBishopMove
        jmp @@sof
    @@Knight:
        call validateKnightMove
        jmp @@sof
    @@Rook:
        call validateRookMove
        jmp @@sof
    @@Queen:
        call validateQueenMove
        jmp @@sof
    @@King:
        call validateKingMove
        jmp @@sof
    @@sof:
    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validateMove

proc validatePawnMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx
    
    push @@sourceX
    push @@sourceY
    call getPieceValue ; AL now contains the piece value

    cmp al, 2
    je @@whitePawn
    ; else
    jmp @@blackPawn

    @@whitePawn:
        mov cx, @@destinationX ; We'll check if he tried to eat a piece (can be diagonally only)
        cmp @@sourceX, cx
        je @@checkWhitePawnMoveForward ; If he didn't change his X value then he only moved forward
        @@whitePawnEatXcheck:
            add cx, 1 ; We'll check if he moved left or right by 1, if he moved by more then it's invalid
            cmp @@sourceX, cx
            je @@whitePawnEatYcheck
            sub cx, 2
            cmp @@sourceX, cx
            jne @@isInvalid
        @@whitePawnEatYcheck:
            ; Check if the Y value is decreased by 1 (which it should be if he moved diagonally and it's valid)
            mov dx, @@destinationY
            add dx, 1
            cmp @@sourceY, dx
            jne @@isInvalid ; If it's still equal after we added by 1 then we know he moved 1 in the Y value, else it's definitely invalid
            ; If we're still here then check if there's a black piece that the pawn can eat
            push @@destinationX
            push @@destinationY
            call getPieceValue
            cmp al, 0
            jge @@isInvalid ; If it's greater (white piece) or equal (EMPTY) then it's invalid
            jmp @@isValid ; If it's not those two then it's a black piece and therefore is valid

        @@checkWhitePawnMoveForward:
            mov dx, @@sourceY ; We'll check whether there's a piece in the way
            sub dx, 1
            push @@sourceX
            push dx
            call getPieceValue
            cmp al, 0
            jne @@isInvalid

            cmp @@sourceY, 6 ; We'll check if it's the pawn's first move, if it is then he can also move by 2 squares
            jne @@checkWhitePawnMoveOneSquare
            @@checkWhitePawnMoveTwoSquares:
                mov dx, @@destinationY
                add dx, 2
                cmp @@sourceY, dx ; If it's equal then we moved two squares
                je @@isValid ; Else, check if we moved only one square
            @@checkWhitePawnMoveOneSquare:
                mov dx, @@destinationY
                add dx, 1
                cmp @@sourceY, dx
                je @@isValid
                jmp @@isInvalid 
    
    @@blackPawn:
        mov cx, @@destinationX ; We'll check if he tried to eat a piece (can be diagonally only)
        cmp @@sourceX, cx
        je @@checkBlackPawnMoveForward ; If he didn't change his X value then he only moved forward
        @@blackPawnEatXcheck:
            add cx, 1 ; We'll check if he moved left or right by 1, if he moved by more then it's invalid
            cmp @@sourceX, cx
            je @@blackPawnEatYcheck
            sub cx, 2
            cmp @@sourceX, cx
            jne @@isInvalid
        @@blackPawnEatYcheck:
            ; Check if the Y value is increased by 1 (which it should be if he moved diagonally and it's valid)
            mov dx, @@destinationY
            sub dx, 1
            cmp @@sourceY, dx
            jne @@isInvalid ; If it's still equal after we added by 1 then we know he moved 1 in the Y value, else it's definetly invalid
            ; If we're still here then check if there's a black piece that the pawn can eat
            push @@destinationX
            push @@destinationY
            call getPieceValue
            cmp al, 0
            jle @@isInvalid ; If it's lesser (black piece) or equal (EMPTY) then it's invalid
            jmp @@isValid ; If it's not those two then it's a white piece and therefore is valid

        @@checkBlackPawnMoveForward:
            mov dx, @@sourceY ; We'll check whether there's a piece in the way
            add dx, 1
            push @@sourceX
            push dx
            call getPieceValue
            cmp al, 0
            jne @@isInvalid

            cmp @@sourceY, 1 ; We'll check if it's the pawn's first move, if it is then he can also move by 2 squares
            jne @@checkBlackPawnMoveOneSquare
            @@checkBlackPawnMoveTwoSquares:
                mov dx, @@destinationY
                sub dx, 2
                cmp @@sourceY, dx ; If it's equal then we moved two squares
                je @@isValid ; Else, check if we moved only one square
            @@checkBlackPawnMoveOneSquare:
                mov dx, @@destinationY
                sub dx, 1
                cmp @@sourceY, dx
                je @@isValid
                jmp @@isInvalid 

    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        ; We'll check whether we need to promote the pawn (currently it's automatically to a queen)
        push @@sourceX
        push @@sourceY
        call getPieceValue ; AL now contains the piece value
        cmp al, 2
        je @@checkWhitePawnPromotion
        ; else
        jmp @@checkBlackPawnPromotion

        @@checkWhitePawnPromotion:
            cmp @@destinationY, 0
            jne @@exit ; If it didn't reach the end of the board, then just continue normally
            push 10 ; White queen value
            push @@sourceX
            push @@sourceY
            call setPieceValue
            jmp @@exit
        @@checkBlackPawnPromotion:
            cmp @@destinationY, 7
            jne @@exit ; If it didn't reach the end of the board, then just continue normally
            push -10 ; Black queen value
            push @@sourceX
            push @@sourceY
            call setPieceValue
            jmp @@exit  
        @@exit:
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validatePawnMove

proc validateBishopMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx

    @@checkMovementWasDiagonal:
    mov ax, @@destinationX
    sub ax, @@sourceX
    cwd ; This gets the absolute value
    xor ax, dx
    sub ax, dx
    mov cx, ax
    mov ax, @@destinationY
    sub ax, @@sourceY
    cwd ; This gets the absolute value
    xor ax, dx
    sub ax, dx
    cmp ax, cx ; If it's diagonal then deltaX and deltaY should be the same
    jne @@isInvalid

    @@checkWhichDiagonal:
        mov cx, @@destinationX
        sub cx, @@sourceX
        cmp cx, 0
        jg @@diagonalRight
        jl @@diagonalLeft

        @@diagonalLeft:
            mov dx, @@destinationY
            sub dx, @@sourceY
            cmp dx, 0
            jl @@diagonalLeftUp
            jg @@diagonalLeftDown

            @@diagonalLeftUp:
                mov cx, @@sourceX
                mov dx, @@sourceY
                @@loopLeftUp:
                    dec cx
                    dec dx
                    push cx
                    push dx
                    call getPieceValue
                    cmp dx, @@destinationY
                    je @@isValid
                    cmp al, 0
                    jne @@isInvalid
                ; If we reached the end without going to invalid then there are no pieces intruding therefore it's valid
                jmp @@loopLeftUp
            @@diagonalLeftDown:
                mov cx, @@sourceX
                mov dx, @@sourceY
                @@loopLeftDown:
                    dec cx
                    inc dx
                    push cx
                    push dx
                    call getPieceValue
                    cmp dx, @@destinationY
                    je @@isValid
                    cmp al, 0
                    jne @@isInvalid
                ; If we reached the end without going to invalid then there are no pieces intruding therefore it's valid
                jmp @@loopLeftDown

        @@diagonalRight:
            mov dx, @@destinationY
            sub dx, @@sourceY
            cmp dx, 0
            jl @@diagonalRightUp
            jg @@diagonalRightDown

            @@diagonalRightUp:
                mov cx, @@sourceX
                mov dx, @@sourceY
                @@loopRightUp:
                    inc cx
                    dec dx
                    push cx
                    push dx
                    call getPieceValue
                    cmp dx, @@destinationY
                    je @@isValid
                    cmp al, 0
                    jne @@isInvalid
                ; If we reached the end without going to invalid then there are no pieces intruding therefore it's valid
                jmp @@loopRightUp
            @@diagonalRightDown:
                mov cx, @@sourceX
                mov dx, @@sourceY
                @@loopRightDown:
                    inc cx
                    inc dx
                    push cx
                    push dx
                    call getPieceValue
                    cmp dx, @@destinationY
                    je @@isValid
                    cmp al, 0
                    jne @@isInvalid
                jmp @@loopRightDown
    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validateBishopMove

proc validateKnightMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx
    
    ; There are two cases where a knight move is valid, case 1: (DeltaX = 2, DeltaY = 1) or case 2: (DeltaX = 1, DeltaY = 2)
    mov ax, @@sourceX
    sub ax, @@destinationX
    cwd ; This gets the absolute value
    xor ax, dx
    sub ax, dx
    mov cx, ax ; Put DeltaX inside CX
    mov ax, @@sourceY
    sub ax, @@destinationY
    cwd ; This gets the absolute value
    xor ax, dx
    sub ax, dx
    mov dx, ax ; Put DeltaY inside DX

    @@case1:
        cmp cx, 2
        jne @@case2
        cmp dx, 1
        je @@isValid
        jmp @@isInvalid
    @@case2:
        cmp cx, 1
        jne @@isInvalid
        cmp dx, 2
        je @@isValid
        jmp @@isInvalid
    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validateKnightMove

proc validateRookMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx

    @@checkLineX:
    mov cx, @@sourceX
    cmp cx, @@destinationX ; Check whether there was a change in the X value
    jne @@checkLineY
    ; Check there wasn't a change in the Y value
    mov dx, @@sourceY
    cmp dx, @@destinationY
    je @@isInvalid ; If both of them changed then it's invalid
    jmp @@checkPathY

    @@checkLineY:
    mov dx, @@sourceY
    cmp dx, @@destinationY
    jne @@isInvalid ; If there was a change in both of them then it's invalid
    jmp @@checkPathX

    @@checkPathY:
        mov dx, @@sourceY
        cmp dx, @@destinationY
        jae @@sourceYisBigger
        @@destinationYisBigger:
            mov dx, @@sourceY
            mov cx, @@destinationY
            jmp @@loopY
        @@sourceYisBigger:
            mov dx, @@destinationY
            mov cx, @@sourceY

        @@loopY:
            inc dx
            push @@sourceX
            push dx
            call getPieceValue
            cmp dx, cx
            je @@isValid
            cmp al, 0
            jne @@isInvalid
        ; If we reached the end without going to invalid then there are no pieces intruding therefore it's valid
        jmp @@loopY

    @@checkPathX:
        mov cx, @@sourceX
        cmp cx, @@destinationX
        jae @@sourceXisBigger
        @@destinationXisBigger:
            mov cx, @@sourceX
            mov dx, @@destinationX
            jmp @@loopX
        @@sourceXisBigger:
            mov cx, @@destinationX
            mov dx, @@sourceX

        @@loopX:
            inc cx
            push cx
            push @@sourceY
            call getPieceValue
            cmp cx, dx
            je @@isValid
            cmp al, 0
            jne @@isInvalid
        ; If we reached the end without going to invalid then there are no pieces intruding therefore it's valid
        jmp @@loopX

    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        ; We'll check which rook moved, and change their variable to "has moved"
        cmp @@sourceY, 7
        je @@isWhiteRook
        cmp @@sourceY, 0
        je @@isBlackRook
        jmp @@dontChangeVariable
        @@isWhiteRook:
            ; Now that we know which color the rook was, we'll check which rook it was (left or right)
            cmp @@sourceX, 7
            je @@isRightWhiteRook
            cmp @@sourceX, 0
            je @@isLeftWhiteRook
            jmp @@dontChangeVariable
            @@isRightWhiteRook:
                mov [rightWhiteRookMoved], 1
                jmp @@dontChangeVariable
            @@isLeftWhiteRook:
                mov [leftWhiteRookMoved], 1
                jmp @@dontChangeVariable
        @@isBlackRook:
            ; Now that we know which color the rook was, we'll check which rook it was (left or right)
            cmp @@sourceX, 7
            je @@isRightBlackRook
            cmp @@sourceX, 0
            je @@isLeftBlackRook
            jmp @@dontChangeVariable
            @@isRightBlackRook:
                mov [rightBlackRookMoved], 1
                jmp @@dontChangeVariable
            @@isLeftBlackRook:
                mov [leftBlackRookMoved], 1
                jmp @@dontChangeVariable
        @@dontChangeVariable:
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validateRookMove

proc validateQueenMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp

    ; A queen is simply either a bishop or a rook, so we'll simply check both and if one is valid then we'll make the queen valid
    @@checkMoveLikeBishop:
        push @@sourceX
        push @@sourceY
        push @@destinationX
        push @@destinationY
        call validateBishopMove
        jnc @@isValid
    @@checkMoveLikeRook:
        push @@sourceX
        push @@sourceY
        push @@destinationX
        push @@destinationY
        call validateRookMove
        jnc @@isValid
    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        clc
    @@sof:

    pop bp
    ret 8
endp validateQueenMove

proc validateKingMove
    @@sourceX equ [word ptr bp + 10]
    @@sourceY equ [word ptr bp + 8]
    @@destinationX equ [word ptr bp + 6]
    @@destinationY equ [word ptr bp + 4]

    push bp
    mov bp, sp
    push ax
    push cx
    push dx

    ; We'll first check if the king tried to castle (we'll check for king's color because it's related to their position)
    push @@sourceX
    push @@sourceY
    call getPieceValue ; AL now contains the piece value

    cmp al, 12
    je @@whiteKing
    ; else
    jmp @@blackKing
    @@whiteKing:
        ; We'll check if he tried to move two squares (that's how we do castling)
        mov ax, @@sourceX
        sub ax, @@destinationX
        cwd
        xor ax, dx
        sub ax, dx
        cmp ax, 2 ; We'll check if he tried to go two squares in the X level
        jne @@generalMove
        cmp [whiteKingMoved], 1 ; Check if the king moved, if he did then he can no longer castle
        je @@isInvalid
        ; Check whether the king is currently in check, if he is then he isn't allowed to castle
        call copyBoardState
        push @@sourceX
        push @@sourceY
        call isInCheck
        jc @@isInvalid
        ; Now that we checked if the king has moved, we'll check which direction he wants to castle
        mov cx, @@sourceX
        cmp cx, @@destinationX
        jb @@whiteCastleRight
        ja @@whiteCastleLeft
        @@whiteCastleRight:
            ; We'll check if the path is clear using rook's move validation (Just to check if the path between the rook and the king is clear)
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call validateRookMove
            jc @@isInvalid
            ; If we're here, then the path is clear and he tried to move two squares, now we only need to check if the rook also didn't move
            cmp [rightWhiteRookMoved], 1
            je @@isInvalid
            ; "Do" the castle in the next board to see if we'll end up in a check after it
            call copyBoardState
            push 7
            push 7
            push 5
            push 7
            call movePieceInNextBoard
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call movePieceInNextBoard
            push @@destinationX
            push @@destinationY
            call isInCheck
            jc @@isInvalid
            ; Now that we checked everything, we can castle
            ; First move the rook
            push 7
            push 7
            push 5
            push 7
            call movePiece
            ; The king will be moved later in the main program
            ; And exit with valid
            jmp @@isValid

        @@whiteCastleLeft:
            ; We'll check if the path is clear using rook's move validation (Just to check if the path between the rook and the king is clear)
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call validateRookMove
            jc @@isInvalid
            ; If we're here, then the path is clear and he tried to move two squares, now we only need to check if the rook also didn't move
            cmp [leftWhiteRookMoved], 1
            je @@isInvalid
            ; "Do" the castle in the next board to see if we'll end up in a check after it
            call copyBoardState
            push 0
            push 7
            push 3
            push 7
            call movePieceInNextBoard
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call movePieceInNextBoard
            push @@destinationX
            push @@destinationY
            call isInCheck
            jc @@isInvalid
            ; Now that we checked everything, we can castle
            ; First move the rook
            push 0
            push 7
            push 3
            push 7
            call movePiece
            ; The king will be moved later in the main program
            ; And exit with valid
            jmp @@isValid

    @@blackKing:
        ; We'll check if he tried to move two squares (that's how we do castling)
        mov ax, @@sourceX
        sub ax, @@destinationX
        cwd
        xor ax, dx
        sub ax, dx
        cmp ax, 2 ; We'll check if he tried to go two squares in the X level
        jne @@generalMove
        cmp [blackKingMoved], 1 ; Check if the king moved, if he did then he can no longer castle
        je @@isInvalid
        call copyBoardState
        push @@sourceX
        push @@sourceY
        call isInCheck
        jc @@isInvalid
        ; Now that we checked if the king has moved, we'll check which direction he wants to castle
        mov cx, @@sourceX
        cmp cx, @@destinationX
        ja @@blackCastleRight
        jb @@blackCastleLeft
        @@blackCastleLeft:
            ; We'll check if the path is clear using rook's move validation (it'll set CARRY flag if it's not clear)
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call validateRookMove
            jc @@isInvalid
            ; If we're here, then the path is clear and he tried to move two squares, now we only need to check if the rook also didn't move
            cmp [leftblackRookMoved], 1
            je @@isInvalid
            ; "Do" the castle in the next board to see if we'll end up in a check after it
            call copyBoardState
            push 7
            push 0
            push 5
            push 0
            call movePieceInNextBoard
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call movePieceInNextBoard
            push @@destinationX
            push @@destinationY
            call isInCheck
            jc @@isInvalid
            ; Now that we checked everything, we can castle
            ; First move the rook
            push 7
            push 0
            push 5
            push 0
            call movePiece
            ; The king will be moved later in the main program
            ; And set the king moved flag
            mov [blackKingMoved], 1
            ; And exit with valid
            jmp @@isValid

        @@blackCastleRight:
            ; We'll check if the path is clear using rook's move validation (it'll set CARRY flag if it's not clear)
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call validateRookMove
            jc @@isInvalid
            ; If we're here, then the path is clear and he tried to move two squares, now we only need to check if the rook also didn't move
            cmp [RightBlackRookMoved], 1
            je @@isInvalid
            ; "Do" the castle in the next board to see if we'll end up in a check after it
            call copyBoardState
            push 0
            push 0
            push 3
            push 0
            call movePieceInNextBoard
            push @@sourceX
            push @@sourceY
            push @@destinationX
            push @@destinationY
            call movePieceInNextBoard
            push @@destinationX
            push @@destinationY
            call isInCheck
            jc @@isInvalid
            ; Now that we checked everything, we can castle
            ; First move the rook
            push 0
            push 0
            push 3
            push 0
            call movePiece
            ; The king will be moved later in the main program
            ; And exit with valid
            jmp @@isValid
    @@generalMove:
    ; In order to validate the king, we simply need to do (DeltaX + DeltaY) <= 2, if it's true then it's valid, else it's invalid
    mov ax, @@sourceX
    sub ax, @@destinationX
    cwd
    xor ax, dx
    sub ax, dx
    mov cx, ax ; CX contains DeltaX
    mov ax, @@sourceY
    sub ax, @@destinationY
    cwd
    xor ax, dx
    sub ax, dx ; AX contains DeltaY

    add ax, cx ; AX = (DeltaX + DeltaY)
    cmp ax, 1
    je @@isValid
    cmp cx, 1
    je @@isValid

    @@isInvalid:
        stc
        jmp @@sof
    @@isValid:
        mov [whiteKingMoved], 1
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 8
endp validateKingMove

proc getKingPosition
    @@playerTurn equ [byte ptr bp + 4]
    @@boardX equ [word ptr bp - 2]
    @@boardY equ [word ptr bp - 4]

    push bp
    mov bp, sp
    sub sp, 4
    push ax
    push si

    mov al, 12
    imul @@playerTurn
    mov ah, al
    
    mov si, offset nextBoard
    mov @@boardY, 0
    mov cx, 8
    @@checkColumnForKing:
        mov @@boardX, 0
        mov dx, 8
        @@checkRowForKing:
            mov al, [byte ptr si] ; Puts the value of the piece so we can check if we found the white king
            cmp al, ah
            je @@kingFound ; If the king is found, check for the black king
            inc si
            add @@boardX, 1
            dec dx
            cmp dx, 0
            jne @@checkRowForKing
        add @@boardY, 1
        loop @@checkColumnForKing

    @@kingFound:
        mov cx, @@boardX
        mov dx, @@boardY
    @@exit:
    pop si
    pop ax
    add sp, 4
    pop bp
    ret 2
endp getKingPosition

; We'll go through every piece of the opponenet's color in the board
; And check for all the legal moves they can do, if it results in the king still being in check.
; If it does check the next move until we finish, if a move results in the king not being in check then it's not checkmate
; If no move results in the king not being in check then it's checkmate.
proc checkForCheckmate
    @@playerTurn equ [byte ptr bp + 4]

    @@boardX equ [word ptr bp - 2]
    @@boardY equ [word ptr bp - 4]

    push bp
    mov bp, sp
    sub sp, 4
    push bx
    push cx
    push dx
    push si

    cmp @@playerTurn, 0
    jg @@checkBlackKingForCheckmate ; If playerTurn is positive then white just did a move so we'll check black isn't in checkmate
    jl @@checkWhiteKingForCheckmate

    @@checkBlackKingForCheckmate:
        mov bx, offset nextBoard
        mov si, 0

        push -1
        call getKingPosition
        push cx
        push dx
        call isInCheck
        jc @@1goThroughBoard
        jmp @@noCheckmate

        @@1goThroughBoard:
            call copyBoardState
            mov al, [byte ptr bx + si]
            cmp al, bPAWN
            je @@1checkPawnMove
            cmp al, bBISHOP
            je @@1checkBishopMove
            cmp al, bKNIGHT
            je @@1checkKnightMove
            cmp al, bROOK
            je @@1checkRookMove
            cmp al, bQUEEN
            je @@1checkQueenMove
            cmp al, bKING
            je @@1checkKingMove
            
            @@1throughBoardContinue:
                inc si
                cmp si, 64
                jne @@1goThroughBoard
                jmp @@blackIsInCheckmate
        
        @@1checkPawnMove:
            @@1checkPawnMoveForward:
                @@1moveForward1:
                    push si
                    call convertFromArrayToBoard ; CX:DX now contain X:Y
                    push cx
                    add dx, 1
                    push dx
                    call getNextBoardPieceValue ; Check there isn't a piece in the way
                    cmp al, 0
                    jne @@1checkPawnEating
                    sub dx, 1
                    push cx
                    push dx
                    add dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push -1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@1moveForward2
                    jmp @@noCheckmate
                @@1moveForward2:
                    push si
                    call convertFromArrayToBoard
                    push cx
                    add dx, 2
                    push dx
                    call getNextBoardPieceValue ; Check there isn't a piece in the way
                    cmp al, 0
                    jne @@1checkPawnEating
                    sub dx, 2
                    cmp dx, 1 ; Check if the pawn is allowed to go 2 squares
                    jne @@1checkPawnEating
                    call copyBoardState
                    push cx
                    push dx
                    add dx, 2
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push -1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@1checkPawnEating
                    jmp @@noCheckmate
            @@1checkPawnEating:
                @@1pawnEatingLeft:
                    push si
                    call convertFromArrayToBoard
                    sub cx, 1
                    add dx, 1
                    cmp cx, 0
                    jl @@1pawnEatingRight ; Check we haven't gone out of bounds
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jle @@1pawnEatingRight
                    add cx, 1
                    sub dx, 1
                    push cx
                    push dx
                    sub cx, 1
                    add dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push -1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@1pawnEatingRight
                    jmp @@noCheckmate
                @@1pawnEatingRight:
                    push si
                    call convertFromArrayToBoard
                    add cx, 1
                    add dx, 1
                    cmp cx, 0
                    jl @@1throughBoardContinue
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jle @@1throughBoardContinue
                    sub cx, 1
                    sub dx, 1
                    push cx
                    push dx
                    add cx, 1
                    add dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push -1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@1throughBoardContinue
                    jmp @@noCheckmate

        @@1checkBishopMove:
            @@1leftUpDiagonal:
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkLeftUpDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    dec dx
                    @@1loopLeftUpDiagonal:
                        cmp cx, 0
                        jl @@1checkRightUpDiagonal
                        cmp dx, 0
                        jl @@1checkRightUpDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1checkRightUpDiagonal
                        je @@1continueLeftUpDiagonal
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1checkRightUpDiagonal
                        jmp @@noCheckmate
                        @@1continueLeftUpDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextLeftUp
                            jmp @@noCheckmate
                            @@1nextLeftUp:
                            call copyBoardState
                            dec cx
                            dec dx
                            jmp @@1loopLeftUpDiagonal
            @@1rightUpDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkRightUpDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    dec dx
                    @@1loopRightUpDiagonal:
                        cmp cx, 7
                        jg @@1leftDownDiagonal
                        cmp dx, 0
                        jl @@1leftDownDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1leftDownDiagonal
                        je @@1continueRightUpDiagonal
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1leftDownDiagonal
                        jmp @@noCheckmate
                        @@1continueRightUpDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextRightUp
                            jmp @@noCheckmate
                            @@1nextRightUp:
                            call copyBoardState
                            inc cx
                            dec dx
                            jmp @@1loopRightUpDiagonal
            @@1leftDownDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkLeftDownDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    inc dx
                    @@1loopLeftDownDiagonal:
                        cmp cx, 0
                        jl @@1rightDownDiagonal
                        cmp dx, 7
                        jg @@1rightDownDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1rightDownDiagonal
                        je @@1continueLeftDownDiagonal
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1rightDownDiagonal
                        jmp @@noCheckmate
                        @@1continueLeftDownDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextLeftDown
                            jmp @@noCheckmate
                            @@1nextLeftDown:
                            call copyBoardState
                            dec cx
                            inc dx
                            jmp @@1loopLeftDownDiagonal

            @@1rightDownDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkRightDownDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    inc dx
                    @@1loopRightDownDiagonal:
                        cmp cx, 7
                        jg @@1bishopDoesntHelp
                        cmp dx, 7
                        jg @@1bishopDoesntHelp
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1bishopDoesntHelp
                        je @@1continueRightDownDiagonal
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1bishopDoesntHelp
                        jmp @@noCheckmate
                        @@1continueRightDownDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextRightDown
                            jmp @@noCheckmate
                            @@1nextRightDown:
                            call copyBoardState
                            inc cx
                            inc dx
                            jmp @@1loopRightDownDiagonal
            @@1bishopDoesntHelp:
                jmp @@1throughBoardContinue

        @@1checkKnightMove:
            push si
            call convertFromArrayToBoard
            mov @@boardX, cx
            mov @@boardY, dx
            @@1checkFirstSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 1
                sub dx, 2
                cmp cx, 0
                jl @@1checkSecondSpot
                cmp dx, 0
                jl @@1checkSecondSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkSecondSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkSecondSpot
                jmp @@noCheckmate
            @@1checkSecondSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 1
                sub dx, 2
                cmp cx, 7
                jg @@1checkThirdSpot
                cmp dx, 0
                jl @@1checkThirdSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkThirdSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkThirdSpot
                jmp @@noCheckmate
            @@1checkThirdSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 2
                sub dx, 1
                cmp cx, 7
                jg @@1checkFourthSpot
                cmp dx, 0
                jl @@1checkFourthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkFourthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkFourthSpot
                jmp @@noCheckmate
            @@1checkFourthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 2
                add dx, 1
                cmp cx, 7
                jg @@1checkFifthSpot
                cmp cx, 7
                jg @@1checkFifthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkFifthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkFifthSpot
                jmp @@noCheckmate
            @@1checkFifthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 1
                add dx, 2
                cmp cx, 7
                jg @@1checkSixthSpot
                cmp dx, 7
                jg @@1checkSixthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkSixthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkSixthSpot
                jmp @@noCheckmate
            @@1checkSixthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 1
                add dx, 2
                cmp cx, 0
                jl @@1checkSeventhSpot
                cmp dx, 7
                jg @@1checkSeventhSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkSeventhSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkSeventhSpot
                jmp @@noCheckmate
            @@1checkSeventhSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 2
                add dx, 1
                cmp cx, 0
                jl @@1checkEighthSpot
                cmp dx, 7
                jg @@1checkEighthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1checkEighthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1checkEighthSpot
                jmp @@noCheckmate
            @@1checkEighthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 2
                sub dx, 1
                cmp cx, 0
                jl @@1knightDoesntHelp
                cmp dx, 0
                jl @@1knightDoesntHelp
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jl @@1knightDoesntHelp
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push -1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@1knightDoesntHelp
                jmp @@noCheckmate
            @@1knightDoesntHelp:
                jmp @@1throughBoardContinue      

        @@1checkRookMove:
            @@1upDirection:
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkUpDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec dx
                    @@1loopUpDirection:
                        cmp dx, 0
                        jl @@1checkRightDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1checkRightDirection
                        je @@1continueUpDirection
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1checkRightDirection
                        jmp @@noCheckmate
                        @@1continueUpDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextUp
                            jmp @@noCheckmate
                            @@1nextUp:
                            call copyBoardState
                            dec dx
                            jmp @@1loopUpDirection
            @@1rightDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkRightDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    @@1loopRightDirection:
                        cmp cx, 7
                        jg @@1downDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1downDirection
                        je @@1continueRightDirection
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1downDirection
                        jmp @@noCheckmate
                        @@1continueRightDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextRight
                            jmp @@noCheckmate
                            @@1nextRight:
                            call copyBoardState
                            inc cx
                            jmp @@1loopRightDirection
            @@1downDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkDownDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc dx
                    @@1loopDownDirection:
                        cmp dx, 7
                        jg @@1leftDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1leftDirection
                        je @@1continueDownDirection
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1leftDirection
                        jmp @@noCheckmate
                        @@1continueDownDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextDown
                            jmp @@noCheckmate
                            @@1nextDown:
                            call copyBoardState
                            inc dx
                            jmp @@1loopDownDirection

            @@1leftDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@1checkLeftDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    @@1loopLeftDirection:
                        cmp cx, 0
                        jl @@1rookDoesntHelp
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jl @@1rookDoesntHelp
                        je @@1continueLeftDirection
                        ; If we find a white piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push -1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@1rookDoesntHelp
                        jmp @@noCheckmate
                        @@1continueLeftDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@1nextLeft
                            jmp @@noCheckmate
                            @@1nextLeft:
                            call copyBoardState
                            dec cx
                            jmp @@1loopLeftDirection
            @@1rookDoesntHelp:
                jmp @@1throughBoardContinue
        
        @@1checkQueenMove:
            @@1queenLikeBishop:
                @@1queenLeftUpDiagonal:
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckLeftUpDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        dec dx
                        @@1queenLoopLeftUpDiagonal:
                            cmp cx, 0
                            jl @@1queenCheckRightUpDiagonal
                            cmp dx, 0
                            jl @@1queenCheckRightUpDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenCheckRightUpDiagonal
                            je @@1queenContinueLeftUpDiagonal
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenCheckRightUpDiagonal
                            jmp @@noCheckmate
                            @@1queenContinueLeftUpDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextLeftUp
                                jmp @@noCheckmate
                                @@1queenNextLeftUp:
                                call copyBoardState
                                dec cx
                                dec dx
                                jmp @@1queenLoopLeftUpDiagonal
                @@1queenRightUpDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckRightUpDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        dec dx
                        @@1queenLoopRightUpDiagonal:
                            cmp cx, 7
                            jg @@1queenLeftDownDiagonal
                            cmp dx, 0
                            jl @@1queenLeftDownDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenLeftDownDiagonal
                            je @@1queenContinueRightUpDiagonal
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenLeftDownDiagonal
                            jmp @@noCheckmate
                            @@1queenContinueRightUpDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextRightUp
                                jmp @@noCheckmate
                                @@1queenNextRightUp:
                                call copyBoardState
                                inc cx
                                dec dx
                                jmp @@1queenLoopRightUpDiagonal
                @@1queenLeftDownDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckLeftDownDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        inc dx
                        @@1queenLoopLeftDownDiagonal:
                            cmp cx, 0
                            jl @@1queenRightDownDiagonal
                            cmp dx, 7
                            jg @@1queenRightDownDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenRightDownDiagonal
                            je @@1queenContinueLeftDownDiagonal
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenRightDownDiagonal
                            jmp @@noCheckmate
                            @@1queenContinueLeftDownDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextLeftDown
                                jmp @@noCheckmate
                                @@1queenNextLeftDown:
                                call copyBoardState
                                dec cx
                                inc dx
                                jmp @@1queenLoopLeftDownDiagonal

                @@1queenRightDownDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckRightDownDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        inc dx
                        @@1queenLoopRightDownDiagonal:
                            cmp cx, 7
                            jg @@1queenBishopDoesntHelp
                            cmp dx, 7
                            jg @@1queenBishopDoesntHelp
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenBishopDoesntHelp
                            je @@1queenContinueRightDownDiagonal
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenBishopDoesntHelp
                            jmp @@noCheckmate
                            @@1queenContinueRightDownDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextRightDown
                                jmp @@noCheckmate
                                @@1queenNextRightDown:
                                call copyBoardState
                                inc cx
                                inc dx
                                jmp @@1queenLoopRightDownDiagonal
                @@1queenBishopDoesntHelp:
                    jmp @@1queenLikeRook

            @@1queenLikeRook:
                @@1queenUpDirection:
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckUpDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec dx
                        @@1queenLoopUpDirection:
                            cmp dx, 0
                            jl @@1queenCheckRightDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenCheckRightDirection
                            je @@1queenContinueUpDirection
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenCheckRightDirection
                            jmp @@noCheckmate
                            @@1queenContinueUpDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextUp
                                jmp @@noCheckmate
                                @@1queenNextUp:
                                call copyBoardState
                                dec dx
                                jmp @@1queenLoopUpDirection
                @@1queenRightDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckRightDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        @@1queenLoopRightDirection:
                            cmp cx, 7
                            jg @@1queenDownDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenDownDirection
                            je @@1queenContinueRightDirection
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenDownDirection
                            jmp @@noCheckmate
                            @@1queenContinueRightDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextRight
                                jmp @@noCheckmate
                                @@1queenNextRight:
                                call copyBoardState
                                inc cx
                                jmp @@1queenLoopRightDirection
                @@1queenDownDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckDownDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc dx
                        @@1queenLoopDownDirection:
                            cmp dx, 7
                            jg @@1queenLeftDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenLeftDirection
                            je @@1queenContinueDownDirection
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenLeftDirection
                            jmp @@noCheckmate
                            @@1queenContinueDownDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextDown
                                jmp @@noCheckmate
                                @@1queenNextDown:
                                call copyBoardState
                                inc dx
                                jmp @@1queenLoopDownDirection

                @@1queenLeftDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@1queenCheckLeftDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        @@1queenLoopLeftDirection:
                            cmp cx, 0
                            jl @@1queenRookDoesntHelp
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jl @@1queenRookDoesntHelp
                            je @@1queenContinueLeftDirection
                            ; If we find a white piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push -1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@1queenRookDoesntHelp
                            jmp @@noCheckmate
                            @@1queenContinueLeftDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push -1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@1queenNextLeft
                                jmp @@noCheckmate
                                @@1queenNextLeft:
                                call copyBoardState
                                dec cx
                                jmp @@1queenLoopLeftDirection
                @@1queenRookDoesntHelp:
                    jmp @@1throughBoardContinue


        @@1checkKingMove:
            push si
            call convertFromArrayToBoard
            mov @@boardX, cx
            mov @@boardY, dx
            dec dx
            mov bx, 3
            @@1checkColumns:
                push bx
                cmp dx, 0
                jl @@1sofCheckRows
                cmp dx, 7
                jg @@1sofCheckRows
                mov cx, @@boardX
                dec cx
                mov bx, 3
                @@1checkRows:
                    cmp cx, 0
                    jl @@1continueLoop
                    cmp cx, 7
                    jg @@1continueLoop
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jl @@1continueLoop
                    push @@boardX
                    push @@boardY
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push cx
                    push dx
                    call isInCheck
                    jc @@1continueLoop
                    pop bx
                    jmp @@noCheckmate

                    @@1continueLoop:
                    inc cx
                    dec bx
                    cmp bx, 0
                    jne @@1checkRows
                @@1sofCheckRows:
                inc dx
                pop bx
                dec bx
                cmp bx, 0
                jne @@1checkColumns
            jmp @@1throughBoardContinue

    @@checkWhiteKingForCheckmate:
        mov bx, offset nextBoard
        mov si, 0

        push 1
        call getKingPosition
        push cx
        push dx
        call isInCheck
        jc @@2goThroughBoard
        jmp @@noCheckmate

        @@2goThroughBoard:
            call copyBoardState
            mov al, [byte ptr bx + si]
            cmp al, wPAWN
            je @@2checkPawnMove
            cmp al, wBISHOP
            je @@2checkBishopMove
            cmp al, wKNIGHT
            je @@2checkKnightMove
            cmp al, wROOK
            je @@2checkRookMove
            cmp al, wQUEEN
            je @@2checkQueenMove
            cmp al, wKING
            je @@2checkKingMove
            
            @@2throughBoardContinue:
                inc si
                cmp si, 64
                jne @@2goThroughBoard
                jmp @@whiteIsInCheckmate
        
        @@2checkPawnMove:
            @@2checkPawnMoveForward:
                @@2moveForward1:
                    push si
                    call convertFromArrayToBoard ; CX:DX now contain X:Y
                    push cx
                    sub dx, 1
                    push dx
                    call getNextBoardPieceValue ; Check there isn't a piece in the way
                    cmp al, 0
                    jne @@2checkPawnEating
                    add dx, 1
                    push cx
                    push dx
                    sub dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push 1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@2moveForward2
                    jmp @@noCheckmate
                @@2moveForward2:
                    push si
                    call convertFromArrayToBoard
                    push cx
                    sub dx, 2
                    push dx
                    call getNextBoardPieceValue ; Check there isn't a piece in the way
                    cmp al, 0
                    jne @@2checkPawnEating
                    add dx, 2
                    cmp dx, 1 ; Check if the pawn is allowed to go 2 squares
                    jne @@2checkPawnEating
                    call copyBoardState
                    push cx
                    push dx
                    sub dx, 2
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push 1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@2checkPawnEating
                    jmp @@noCheckmate
            @@2checkPawnEating:
                @@2pawnEatingLeft:
                    push si
                    call convertFromArrayToBoard
                    sub cx, 1
                    sub dx, 1
                    cmp cx, 0
                    jl @@2pawnEatingRight ; Check we haven't gone out of bounds
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jge @@2pawnEatingRight
                    add cx, 1
                    add dx, 1
                    push cx
                    push dx
                    sub cx, 1
                    sub dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push 1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@2pawnEatingRight
                    jmp @@noCheckmate
                @@2pawnEatingRight:
                    push si
                    call convertFromArrayToBoard
                    add cx, 1
                    sub dx, 1
                    cmp cx, 0
                    jl @@2throughBoardContinue
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jge @@2throughBoardContinue
                    sub cx, 1
                    add dx, 1
                    push cx
                    push dx
                    add cx, 1
                    sub dx, 1
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push 1
                    call getKingPosition
                    push cx
                    push dx
                    call isInCheck
                    jc @@2throughBoardContinue
                    jmp @@noCheckmate

        @@2checkBishopMove:
            @@2leftUpDiagonal:
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkLeftUpDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    dec dx
                    @@2loopLeftUpDiagonal:
                        cmp cx, 0
                        jl @@2checkRightUpDiagonal
                        cmp dx, 0
                        jl @@2checkRightUpDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2checkRightUpDiagonal
                        je @@2continueLeftUpDiagonal
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2checkRightUpDiagonal
                        jmp @@noCheckmate
                        @@2continueLeftUpDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextLeftUp
                            jmp @@noCheckmate
                            @@2nextLeftUp:
                            call copyBoardState
                            dec cx
                            dec dx
                            jmp @@2loopLeftUpDiagonal
            @@2rightUpDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkRightUpDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    dec dx
                    @@2loopRightUpDiagonal:
                        cmp cx, 7
                        jg @@2leftDownDiagonal
                        cmp dx, 0
                        jl @@2leftDownDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2leftDownDiagonal
                        je @@2continueRightUpDiagonal
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2leftDownDiagonal
                        jmp @@noCheckmate
                        @@2continueRightUpDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextRightUp
                            jmp @@noCheckmate
                            @@2nextRightUp:
                            call copyBoardState
                            inc cx
                            dec dx
                            jmp @@2loopRightUpDiagonal
            @@2leftDownDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkLeftDownDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    inc dx
                    @@2loopLeftDownDiagonal:
                        cmp cx, 0
                        jl @@2rightDownDiagonal
                        cmp dx, 7
                        jg @@2rightDownDiagonal
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2rightDownDiagonal
                        je @@2continueLeftDownDiagonal
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2rightDownDiagonal
                        jmp @@noCheckmate
                        @@2continueLeftDownDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextLeftDown
                            jmp @@noCheckmate
                            @@2nextLeftDown:
                            call copyBoardState
                            dec cx
                            inc dx
                            jmp @@2loopLeftDownDiagonal

            @@2rightDownDiagonal:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkRightDownDiagonal:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    inc dx
                    @@2loopRightDownDiagonal:
                        cmp cx, 7
                        jg @@2bishopDoesntHelp
                        cmp dx, 7
                        jg @@2bishopDoesntHelp
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2bishopDoesntHelp
                        je @@2continueRightDownDiagonal
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2bishopDoesntHelp
                        jmp @@noCheckmate
                        @@2continueRightDownDiagonal:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextRightDown
                            jmp @@noCheckmate
                            @@2nextRightDown:
                            call copyBoardState
                            inc cx
                            inc dx
                            jmp @@2loopRightDownDiagonal
            @@2bishopDoesntHelp:
                jmp @@2throughBoardContinue

        @@2checkKnightMove:
            push si
            call convertFromArrayToBoard
            mov @@boardX, cx
            mov @@boardY, dx
            @@2checkFirstSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 1
                sub dx, 2
                cmp cx, 0
                jl @@2checkSecondSpot
                cmp dx, 0
                jl @@2checkSecondSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkSecondSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkSecondSpot
                jmp @@noCheckmate
            @@2checkSecondSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 1
                sub dx, 2
                cmp cx, 7
                jg @@2checkThirdSpot
                cmp dx, 0
                jl @@2checkThirdSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkThirdSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkThirdSpot
                jmp @@noCheckmate
            @@2checkThirdSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 2
                sub dx, 1
                cmp cx, 7
                jg @@2checkFourthSpot
                cmp dx, 0
                jl @@2checkFourthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkFourthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkFourthSpot
                jmp @@noCheckmate
            @@2checkFourthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 2
                add dx, 1
                cmp cx, 7
                jg @@2checkFifthSpot
                cmp cx, 7
                jg @@2checkFifthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkFifthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkFifthSpot
                jmp @@noCheckmate
            @@2checkFifthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                add cx, 1
                add dx, 2
                cmp cx, 7
                jg @@2checkSixthSpot
                cmp dx, 7
                jg @@2checkSixthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkSixthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkSixthSpot
                jmp @@noCheckmate
            @@2checkSixthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 1
                add dx, 2
                cmp cx, 0
                jl @@2checkSeventhSpot
                cmp dx, 7
                jg @@2checkSeventhSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkSeventhSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkSeventhSpot
                jmp @@noCheckmate
            @@2checkSeventhSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 2
                add dx, 1
                cmp cx, 0
                jl @@2checkEighthSpot
                cmp dx, 7
                jg @@2checkEighthSpot
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2checkEighthSpot
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2checkEighthSpot
                jmp @@noCheckmate
            @@2checkEighthSpot:
                call copyBoardState
                mov cx, @@boardX
                mov dx, @@boardY
                sub cx, 2
                sub dx, 1
                cmp cx, 0
                jl @@2knightDoesntHelp
                cmp dx, 0
                jl @@2knightDoesntHelp
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0
                jg @@2knightDoesntHelp
                push @@boardX
                push @@boardY
                push cx
                push dx
                call movePieceInNextBoard
                push 1
                call getKingPosition
                push cx
                push dx
                call isInCheck
                jc @@2knightDoesntHelp
                jmp @@noCheckmate
            @@2knightDoesntHelp:
                jmp @@2throughBoardContinue
            

        @@2checkRookMove:
            @@2upDirection:
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkUpDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec dx
                    @@2loopUpDirection:
                        cmp dx, 0
                        jl @@2checkRightDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2checkRightDirection
                        je @@2continueUpDirection
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2checkRightDirection
                        jmp @@noCheckmate
                        @@2continueUpDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextUp
                            jmp @@noCheckmate
                            @@2nextUp:
                            call copyBoardState
                            dec dx
                            jmp @@2loopUpDirection
            @@2rightDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkRightDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc cx
                    @@2loopRightDirection:
                        cmp cx, 7
                        jg @@2downDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2downDirection
                        je @@2continueRightDirection
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2downDirection
                        jmp @@noCheckmate
                        @@2continueRightDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextRight
                            jmp @@noCheckmate
                            @@2nextRight:
                            call copyBoardState
                            inc cx
                            jmp @@2loopRightDirection
            @@2downDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkDownDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    inc dx
                    @@2loopDownDirection:
                        cmp dx, 7
                        jg @@2leftDirection
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2leftDirection
                        je @@2continueDownDirection
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2leftDirection
                        jmp @@noCheckmate
                        @@2continueDownDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextDown
                            jmp @@noCheckmate
                            @@2nextDown:
                            call copyBoardState
                            inc dx
                            jmp @@2loopDownDirection

            @@2leftDirection:
                call copyBoardState
                push si
                call convertFromArrayToBoard
                mov @@boardX, cx
                mov @@boardY, dx
                @@2checkLeftDirection:
                    mov cx, @@boardX
                    mov dx, @@boardY
                    dec cx
                    @@2loopLeftDirection:
                        cmp cx, 0
                        jl @@2rookDoesntHelp
                        push cx
                        push dx
                        call getNextBoardPieceValue
                        cmp al, 0
                        jg @@2rookDoesntHelp
                        je @@2continueLeftDirection
                        ; If we find a black piece then we can't go any further than eating it.
                        call copyBoardState
                        push @@boardX
                        push @@boardY
                        push cx
                        push dx
                        call movePieceInNextBoard
                        push 1
                        call getKingPosition
                        push cx
                        push dx
                        call isInCheck
                        jc @@2rookDoesntHelp
                        jmp @@noCheckmate
                        @@2continueLeftDirection:
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                                push cx
                                push dx
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                                pop dx
                                pop cx
                            jc @@2nextLeft
                            jmp @@noCheckmate
                            @@2nextLeft:
                            call copyBoardState
                            dec cx
                            jmp @@2loopLeftDirection
            @@2rookDoesntHelp:
                jmp @@2throughBoardContinue
        
        @@2checkQueenMove:
            @@2queenLikeBishop:
                @@2queenLeftUpDiagonal:
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckLeftUpDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        dec dx
                        @@2queenLoopLeftUpDiagonal:
                            cmp cx, 0
                            jl @@2queenCheckRightUpDiagonal
                            cmp dx, 0
                            jl @@2queenCheckRightUpDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenCheckRightUpDiagonal
                            je @@2queenContinueLeftUpDiagonal
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenCheckRightUpDiagonal
                            jmp @@noCheckmate
                            @@2queenContinueLeftUpDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextLeftUp
                                jmp @@noCheckmate
                                @@2queenNextLeftUp:
                                call copyBoardState
                                dec cx
                                dec dx
                                jmp @@2queenLoopLeftUpDiagonal
                @@2queenRightUpDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckRightUpDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        dec dx
                        @@2queenLoopRightUpDiagonal:
                            cmp cx, 7
                            jg @@2queenLeftDownDiagonal
                            cmp dx, 0
                            jl @@2queenLeftDownDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenLeftDownDiagonal
                            je @@2queenContinueRightUpDiagonal
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenLeftDownDiagonal
                            jmp @@noCheckmate
                            @@2queenContinueRightUpDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextRightUp
                                jmp @@noCheckmate
                                @@2queenNextRightUp:
                                call copyBoardState
                                inc cx
                                dec dx
                                jmp @@2queenLoopRightUpDiagonal
                @@2queenLeftDownDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckLeftDownDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        inc dx
                        @@2queenLoopLeftDownDiagonal:
                            cmp cx, 0
                            jl @@2queenRightDownDiagonal
                            cmp dx, 7
                            jg @@2queenRightDownDiagonal
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenRightDownDiagonal
                            je @@2queenContinueLeftDownDiagonal
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenRightDownDiagonal
                            jmp @@noCheckmate
                            @@2queenContinueLeftDownDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextLeftDown
                                jmp @@noCheckmate
                                @@2queenNextLeftDown:
                                call copyBoardState
                                dec cx
                                inc dx
                                jmp @@2queenLoopLeftDownDiagonal

                @@2queenRightDownDiagonal:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckRightDownDiagonal:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        inc dx
                        @@2queenLoopRightDownDiagonal:
                            cmp cx, 7
                            jg @@2queenBishopDoesntHelp
                            cmp dx, 7
                            jg @@2queenBishopDoesntHelp
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenBishopDoesntHelp
                            je @@2queenContinueRightDownDiagonal
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenBishopDoesntHelp
                            jmp @@noCheckmate
                            @@2queenContinueRightDownDiagonal:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextRightDown
                                jmp @@noCheckmate
                                @@2queenNextRightDown:
                                call copyBoardState
                                inc cx
                                inc dx
                                jmp @@2queenLoopRightDownDiagonal
                @@2queenBishopDoesntHelp:
                    jmp @@2queenLikeRook

            @@2queenLikeRook:
                @@2queenUpDirection:
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckUpDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec dx
                        @@2queenLoopUpDirection:
                            cmp dx, 0
                            jl @@2queenCheckRightDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenCheckRightDirection
                            je @@2queenContinueUpDirection
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenCheckRightDirection
                            jmp @@noCheckmate
                            @@2queenContinueUpDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextUp
                                jmp @@noCheckmate
                                @@2queenNextUp:
                                call copyBoardState
                                dec dx
                                jmp @@2queenLoopUpDirection
                @@2queenRightDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckRightDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc cx
                        @@2queenLoopRightDirection:
                            cmp cx, 7
                            jg @@2queenDownDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenDownDirection
                            je @@2queenContinueRightDirection
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenDownDirection
                            jmp @@noCheckmate
                            @@2queenContinueRightDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextRight
                                jmp @@noCheckmate
                                @@2queenNextRight:
                                call copyBoardState
                                inc cx
                                jmp @@2queenLoopRightDirection
                @@2queenDownDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckDownDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        inc dx
                        @@2queenLoopDownDirection:
                            cmp dx, 7
                            jg @@2queenLeftDirection
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenLeftDirection
                            je @@2queenContinueDownDirection
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenLeftDirection
                            jmp @@noCheckmate
                            @@2queenContinueDownDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextDown
                                jmp @@noCheckmate
                                @@2queenNextDown:
                                call copyBoardState
                                inc dx
                                jmp @@2queenLoopDownDirection

                @@2queenLeftDirection:
                    call copyBoardState
                    push si
                    call convertFromArrayToBoard
                    mov @@boardX, cx
                    mov @@boardY, dx
                    @@2queenCheckLeftDirection:
                        mov cx, @@boardX
                        mov dx, @@boardY
                        dec cx
                        @@2queenLoopLeftDirection:
                            cmp cx, 0
                            jl @@2queenRookDoesntHelp
                            push cx
                            push dx
                            call getNextBoardPieceValue
                            cmp al, 0
                            jg @@2queenRookDoesntHelp
                            je @@2queenContinueLeftDirection
                            ; If we find a black piece then we can't go any further than eating it.
                            call copyBoardState
                            push @@boardX
                            push @@boardY
                            push cx
                            push dx
                            call movePieceInNextBoard
                            push 1
                            call getKingPosition
                            push cx
                            push dx
                            call isInCheck
                            jc @@2queenRookDoesntHelp
                            jmp @@noCheckmate
                            @@2queenContinueLeftDirection:
                                push @@boardX
                                push @@boardY
                                push cx
                                push dx
                                call movePieceInNextBoard
                                    push cx
                                    push dx
                                push 1
                                call getKingPosition
                                push cx
                                push dx
                                call isInCheck
                                    pop dx
                                    pop cx
                                jc @@2queenNextLeft
                                jmp @@noCheckmate
                                @@2queenNextLeft:
                                call copyBoardState
                                dec cx
                                jmp @@2queenLoopLeftDirection
                @@2queenRookDoesntHelp:
                    jmp @@2throughBoardContinue

        @@2checkKingMove:
            push si
            call convertFromArrayToBoard
            mov @@boardX, cx
            mov @@boardY, dx
            dec dx
            mov bx, 3
            @@2checkColumns:
                push bx
                cmp dx, 0
                jl @@2sofCheckRows
                cmp dx, 7
                jg @@2sofCheckRows
                mov cx, @@boardX
                dec cx
                mov bx, 3
                @@2checkRows:
                    cmp cx, 0
                    jl @@2continueLoop
                    cmp cx, 7
                    jg @@2continueLoop
                    call copyBoardState
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, 0
                    jg @@2continueLoop
                    push @@boardX
                    push @@boardY
                    push cx
                    push dx
                    call movePieceInNextBoard
                    push cx
                    push dx
                    call isInCheck
                    jc @@2continueLoop
                    pop bx
                    jmp @@noCheckmate

                    @@2continueLoop:
                    inc cx
                    dec bx
                    cmp bx, 0
                    jne @@2checkRows
                @@2sofCheckRows:
                inc dx
                pop bx
                dec bx
                cmp bx, 0
                jne @@2checkColumns
            jmp @@2throughBoardContinue

    @@blackIsInCheckmate:
        mov al, 1
        jmp @@sof
    @@whiteIsInCheckmate:
        mov al, -1
        jmp @@sof
    @@noCheckmate:
        mov al, 0
    @@sof:

    pop si
    pop dx
    pop cx
    pop bx
    add sp, 4
    pop bp
    ret 2
endp checkForCheckmate

proc movePiece

    @@sourceX equ [word ptr bp + 10] ; In relation to the board, not the screen!
    @@sourceY equ [word ptr bp + 8] ; In relation to the board, not the screen!
    @@destinationX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@destinationY equ [word ptr bp + 4] ; In relation to the board, not the screen!
    
    push bp
    mov bp, sp
    push ax
    push bx
    push cx
    push dx

    ; Saves the value at the board
    push @@sourceX
    push @@sourceY
    call getPieceValue ; Saves the piece value in AL

    ; Clears the value at the source location
    push EMPTY
    push @@sourceX
    push @@sourceY
    call setPieceValue

    ; Sets the value at the board to previously saved value
    push ax
    push @@destinationX
    push @@destinationY
    call setPieceValue

    ; This clears the source's square
    ; We'll calculate the color by using (9 * Y) + X, and then checking whether it's even or odd
    mov bx, 9
    imul bx, @@sourceY
    add bx, @@sourceX
    and bx, 0001b ; This gives us the parity except its the opposite bit
    xor bx, 0001b ; This fixes it
    mov bl, [byte ptr bx + squareColors]
    xor bh, bh
    push bx ; This will push the color

    push @@sourceX
    push @@sourceY
    call drawSquare
    
    ; Now we'll repeat it the above but for the destination square

    ; We'll calculate the color by using (9 * Y) + X, and then checking whether it's even or odd
    mov bx, 9
    imul bx, @@destinationY
    add bx, @@destinationX
    and bx, 0001b ; This gives us the parity except its the opposite bit
    xor bx, 0001b ; This fixes it
    mov bl, [byte ptr bx + squareColors]
    xor bh, bh
    push bx ; This will push the color

    push @@destinationX
    push @@destinationY
    call drawSquare

    ; Now that both squares are clear we can draw the piece at the destination
    push ax
    push @@destinationX
    push @@destinationY
    call drawPiece

    pop dx
    pop cx
    pop bx
    pop ax
    pop bp
    ret 8
endp movePiece

proc getPieceValue
    @@boardX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@boardY equ [word ptr bp + 4] ; In relation to the board, not the screen!

    push bp
    mov bp, sp
    push bx

    mov bx, @@boardY
    imul bx, 8
    add bx, @@boardX
    xor ah, ah
    mov al, [byte ptr bx + board]

    pop bx
    pop bp
    ret 4
endp getPieceValue

proc setPieceValue
    @@wantedPieceValue equ [byte ptr bp + 8]
    @@boardX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@boardY equ [word ptr bp + 4] ; In relation to the board, not the screen!

    push bp
    mov bp, sp
    push ax
    push bx

    mov bx, @@boardY
    imul bx, 8
    add bx, @@boardX
    mov al, @@wantedPieceValue
    mov [byte ptr bx + board], al

    pop bx
    pop ax
    pop bp
    ret 6
endp setPieceValue

proc isInCheck
    @@boardX equ [word ptr bp + 6]
    @@boardY equ [word ptr bp + 4]
    push bp
    mov bp, sp
    push ax
    push cx
    push dx

    push @@boardX
    push @@boardY
    call getNextBoardPieceValue

    cmp al, 0
    jg @@WhiteKing
    jl @@BlackKing
    jmp @@isInCheck

    @@WhiteKing:
        @@checkForBlackPawns: ; the only possible places a pawn can threaten a king is diagonal up-left and up-right
            mov cx, @@boardX
            mov dx, @@boardY
            sub dx, 1 ; Go up one row in the board
            @@checkLeftPawn:
            sub cx, 1 ; We'll check if there's a pawn to the left and then to the right
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bPAWN
            jne @@1checkRightPawn
            jmp @@isInCheck

            @@1checkRightPawn:
            add cx, 2
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bPAWN
            jne @@checkForBlackBishops
            jmp @@isInCheck

        @@checkForBlackBishops: ; We'll check all the 4 diagonals until we reach a white piece (blocking the diagonal) or the board's end, if we find a black bishop/queen then we're in check
            mov cx, @@boardX
            mov dx, @@boardY
			cmp cx, 0
			je @@1sofLeftUp
			cmp dx, 0
			je @@1sofLeftUp
            @@1checkLeftUpDiagonal:
                dec cx
				dec dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a white piece is there, go check the next diagonal
				jg @@1sofLeftUp
				cmp al, bBISHOP
				jne @@1checkQueenLeftUp
                jmp @@isInCheck
                @@1checkQueenLeftUp:
				cmp al, bQUEEN
				jne @@1continueLeftUpDiagonal
                jmp @@isInCheck
				@@1continueLeftUpDiagonal:
                cmp al, 0
                jne @@1sofLeftUp
				cmp cx, 0
				je @@1sofLeftUp
				cmp dx, 0
				je @@1sofLeftUp
				jmp @@1checkLeftUpDiagonal
			
			@@1sofLeftUp:
			mov cx, @@boardX
			mov dx, @@boardY
			cmp cx, 7
			je @@1sofRightUp
			cmp dx, 0
			je @@1sofRightUp
			@@1checkRightUpDiagonal:
				inc cx
				dec dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a white piece is there, go check the next diagonal
				jg @@1sofRightUp
				cmp al, bBISHOP
				jne @@1checkQueenRightUp
                jmp @@isInCheck
                @@1checkQueenRightUp:
				cmp al, bQUEEN
				jne @@1continueRightUpDiagonal
                jmp @@isInCheck
				@@1continueRightUpDiagonal:
                cmp al, 0
                jne @@1sofRightUp
				cmp cx, 7
				je @@1sofRightUp
				cmp dx, 0
				je @@1sofRightUp
				jmp @@1checkRightUpDiagonal
				
			@@1sofRightUp:
			mov cx, @@boardX
			mov dx, @@boardY
			cmp cx, 0
			je @@1sofLeftDown
			cmp dx, 7
			je @@1sofLeftDown
			@@1checkLeftDownDiagonal:
				dec cx
				inc dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a white piece is there, go check the next diagonal
				jg @@1sofLeftDown
				cmp al, bBISHOP
				jne @@1checkQueenLeftDown
                jmp @@isInCheck
                @@1checkQueenLeftDown:
				cmp al, bQUEEN
				jne @@1continueLeftDownDiagonal
                jmp @@isInCheck
				@@1continueLeftDownDiagonal:
                cmp al, 0
                jne @@1sofLeftDown
				cmp cx, 0
				je @@1sofLeftDown
				cmp dx, 7
				je @@1sofLeftDown
				jmp @@1checkLeftDownDiagonal
			@@1sofLeftDown:
			mov cx, @@boardX
            mov dx, @@boardY
            cmp cx, 7
            je @@checkForBlackKnights
            cmp dx, 7
            je @@checkForBlackKnights
			@@1checkRightDownDiagonal:
                inc cx
                inc dx
                push cx
                push dx
                call getNextBoardPieceValue
				cmp al, 0 ; If a white piece is there, go check the next diagonal
				jg @@checkForBlackKnights
				cmp al, bBISHOP
				jne @@1checkQueenRightDown
                jmp @@isInCheck
                @@1checkQueenRightDown:
				cmp al, bQUEEN
				jne @@1continueRightDownDiagonal
                jmp @@isInCheck
				@@1continueRightDownDiagonal:
                cmp al, 0
                jne @@checkForBlackKnights
				cmp cx, 7
				je @@checkForBlackKnights
				cmp dx, 7
				je @@checkForBlackKnights
				jmp @@1checkRightDownDiagonal

        @@checkForBlackKnights: ; A knight has 8 possible spots to attack so we'll check each of them
            @@1checkFirstSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 1
            sub dx, 2
            cmp cx, 0
            jl @@1checkSecondSpot
            cmp dx, 0
            jl @@1checkSecondSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkSecondSpot
            jmp @@isInCheck
            @@1checkSecondSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 1
            sub dx, 2
            cmp cx, 7
            jg @@1checkThirdSpot
            cmp dx, 0
            jl @@1checkThirdSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkThirdSpot
            jmp @@isInCheck
            @@1checkThirdSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 2
            sub dx, 1
            cmp cx, 7
            jg @@1checkFourthSpot
            cmp dx, 0
            jl @@1checkFourthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkFourthSpot
            jmp @@isInCheck
            @@1checkFourthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 2
            add dx, 1
            cmp cx, 7
            jg @@1checkFifthSpot
            cmp dx, 7
            jg @@1checkFifthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkFifthSpot
            jmp @@isInCheck
            @@1checkFifthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 1
            add dx, 2
            cmp cx, 7
            jg @@1checkSixthSpot
            cmp dx, 7
            jg @@1checkSixthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkSixthSpot
            jmp @@isInCheck
            @@1checkSixthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 1
            add dx, 2
            cmp cx, 0
            jl @@1checkSeventhSpot
            cmp dx, 7
            jg @@1checkSeventhSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkSeventhSpot
            jmp @@isInCheck
            @@1checkSeventhSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 2
            add dx, 1
            cmp cx, 0
            jl @@1checkEighthSpot
            cmp dx, 7
            jg @@1checkEighthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@1checkEighthSpot
            jmp @@isInCheck
            @@1checkEighthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 2
            sub dx, 1
            cmp cx, 0
            jl @@checkForBlackRooks
            cmp dx, 0
            jl @@checkForBlackRooks
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, bKNIGHT
            jne @@checkForBlackRooks
            jmp @@isInCheck
        @@checkForBlackRooks: ; We'll check the 4 cardinal directions and if we find a rook/queen there, with no pieces intruding then we're in check 
            mov cx, @@boardX
            mov dx, @@boardY
            cmp dx, 0
            je @@1sofUp
            @@1checkUp:
                dec dx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a white piece is there, go check the next Direction
                jg @@1sofUp
                cmp al, bROOK
				jne @@1checkQueenUp
                jmp @@isInCheck
                @@1checkQueenUp:
				cmp al, bQUEEN
				jne @@1continueUpDirection
                jmp @@isInCheck
				@@1continueUpDirection:
                cmp al, 0
                jne @@1sofUp
                cmp dx, 0
                jne @@1checkUp

            @@1sofUp:
            mov dx, @@boardY
            cmp cx, 7
            je @@1sofRight
            @@1checkRight:
                inc cx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a white piece is there, go check the next Direction
                jg @@1sofRight
                cmp al, bROOK
				jne @@1checkQueenRight
                jmp @@isInCheck
                @@1checkQueenRight:
				cmp al, bQUEEN
				jne @@1continueRightDirection
                jmp @@isInCheck
				@@1continueRightDirection:
                cmp al, 0
                jne @@1sofRight
                cmp cx, 7
                jne @@1checkRight

            @@1sofRight:
            mov cx, @@boardX
            cmp dx, 7
            je @@1sofDown
            @@1checkDown:
                inc dx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a white piece is there, go check the next Direction
                jg @@1sofDown
                cmp al, bROOK
				jne @@1checkQueenDown
                jmp @@isInCheck
                @@1checkQueenDown:
				cmp al, bQUEEN
				jne @@1continueDownDirection
                jmp @@isInCheck
				@@1continueDownDirection:
                cmp al, 0
                jne @@1sofDown
                cmp dx, 7
                jne @@1checkDown

            @@1sofDown:
            mov dx, @@boardY
            cmp cx, 0
            je @@checkForBlackKing
            @@1checkLeft:
                dec cx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a white piece is there, go check the next Direction
                jg @@checkForBlackKing
                cmp al, bROOK
				jne @@1checkQueenLeft
                jmp @@isInCheck
                @@1checkQueenLeft:
				cmp al, bQUEEN
				jne @@1continueLeftDirection
                jmp @@isInCheck
				@@1continueLeftDirection:
                cmp al, 0
                jne @@checkForBlackKing
                cmp cx, 0
                jne @@1checkLeft
                
        @@checkForBlackKing: ; We'll check a square around to see if the opponent's king is there
            mov dx, @@boardY
            dec dx
            mov bx, 3
            @@1checkLoop:
                push bx
                cmp dx, 0
                jl @@1continueLoop
                cmp dx, 7
                jg @@1continueLoop
                mov cx, @@boardX
                dec cx
                mov bx, 3
                @@1secondCheckLoop:
                    cmp cx, 0
                    jl @@1continueLoop
                    cmp cx, 7
                    jg @@1continueLoop
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, bKING
                    jne @@1continueLoop
                    pop bx
                    jmp @@isInCheck

                    @@1continueLoop:
                    inc cx
                    dec bx
                    cmp bx, 0
                    jne @@1secondCheckLoop
                inc dx
                pop bx
                dec bx
                cmp bx, 0
                jne @@1checkLoop
                
        ; We checked every possible piece that can threaten the king, if none of them can eat him then he's not in check.
        jmp @@isNotInCheck
    @@BlackKing:
        @@checkForWhitePawns: ; the only possible places a pawn can threaten a king is diagonal up-left and up-right
            mov cx, @@boardX
            mov dx, @@boardY
            add dx, 1 ; Go up one row in the board
            @@2checkLeftPawn:
            sub cx, 1 ; We'll check if there's a pawn to the left and then to the right
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wPAWN
            jne @@2checkRightPawn
            jmp @@isInCheck

            @@2checkRightPawn:
            add cx, 2
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wPAWN
            jne @@checkForWhiteBishops
            jmp @@isInCheck

        @@checkForWhiteBishops: ; We'll check all the 4 diagonals until we reach a Black piece (blocking the diagonal) or the board's end, if we find a White bishop/queen then we're in check
            mov cx, @@boardX
            mov dx, @@boardY
			cmp cx, 0
			je @@2sofLeftUp
			cmp dx, 0
			je @@2sofLeftUp
            @@2checkLeftUpDiagonal:
                dec cx
				dec dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a Black piece is there, go check the next diagonal
				jl @@2sofLeftUp
				cmp al, wBISHOP
				jne @@2checkQueenLeftUp
                jmp @@isInCheck
                @@2checkQueenLeftUp:
				cmp al, wQUEEN
				jne @@2continueLeftUpDiagonal
                jmp @@isInCheck
				@@2continueLeftUpDiagonal:
                cmp al, 0
                jne @@2sofLeftUp
				cmp cx, 0
				je @@2sofLeftUp
				cmp dx, 0
				je @@2sofLeftUp
				jmp @@2checkLeftUpDiagonal
			
			@@2sofLeftUp:
			mov cx, @@boardX
			mov dx, @@boardY
			cmp cx, 7
			je @@2sofRightUp
			cmp dx, 0
			je @@2sofRightUp
			@@2checkRightUpDiagonal:
				inc cx
				dec dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a Black piece is there, go check the next diagonal
				jl @@2sofRightUp
				cmp al, wBISHOP
				jne @@2checkQueenRightUp
                jmp @@isInCheck
                @@2checkQueenRightUp:
				cmp al, wQUEEN
				jne @@2continueRightUpDiagonal
                jmp @@isInCheck
				@@2continueRightUpDiagonal:
                cmp al, 0
                jne @@2sofRightUp
				cmp cx, 7
				je @@2sofRightUp
				cmp dx, 0
				je @@2sofRightUp
				jmp @@2checkRightUpDiagonal
				
			@@2sofRightUp:
			mov cx, @@boardX
			mov dx, @@boardY
			cmp cx, 0
			je @@2sofLeftDown
			cmp dx, 7
			je @@2sofLeftDown
			@@2checkLeftDownDiagonal:
				dec cx
				inc dx
				push cx
				push dx
				call getNextBoardPieceValue
				cmp al, 0 ; If a Black piece is there, go check the next diagonal
				jl @@2sofLeftDown
				cmp al, wBISHOP
				jne @@2checkQueenLeftDown
                jmp @@isInCheck
                @@2checkQueenLeftDown:
				cmp al, wQUEEN
				jne @@2continueLeftDownDiagonal
                jmp @@isInCheck
				@@2continueLeftDownDiagonal:
                cmp al, 0
                jne @@2sofLeftDown
				cmp cx, 0
				je @@2sofLeftDown
				cmp dx, 7
				je @@2sofLeftDown
				jmp @@2checkLeftDownDiagonal
			@@2sofLeftDown:
			mov cx, @@boardX
            mov dx, @@boardY
            cmp cx, 7
            je @@checkForWhiteKnights
            cmp dx, 7
            je @@checkForWhiteKnights
			@@2checkRightDownDiagonal:
                inc cx
                inc dx
                push cx
                push dx
                call getNextBoardPieceValue
				cmp al, 0 ; If a Black piece is there, go check the next diagonal
				jl @@checkForWhiteKnights
				cmp al, wBISHOP
				jne @@2checkQueenRightDown
                jmp @@isInCheck
                @@2checkQueenRightDown:
				cmp al, wQUEEN
				jne @@2continueRightDownDiagonal
                jmp @@isInCheck
				@@2continueRightDownDiagonal:
                cmp al, 0
                jne @@checkForWhiteKnights
				cmp cx, 7
				je @@checkForWhiteKnights
				cmp dx, 7
				je @@checkForWhiteKnights
				jmp @@2checkRightDownDiagonal

        @@checkForWhiteKnights: ; A knight has 8 possible spots to attack so we'll check each of them
            @@2checkFirstSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 1
            sub dx, 2
            cmp cx, 0
            jl @@2checkSecondSpot
            cmp dx, 0
            jl @@2checkSecondSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkSecondSpot
            jmp @@isInCheck
            @@2checkSecondSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 1
            sub dx, 2
            cmp cx, 7
            jg @@2checkThirdSpot
            cmp dx, 0
            jl @@2checkThirdSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkThirdSpot
            jmp @@isInCheck
            @@2checkThirdSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 2
            sub dx, 1
            cmp cx, 7
            jg @@2checkFourthSpot
            cmp dx, 0
            jl @@2checkFourthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkFourthSpot
            jmp @@isInCheck
            @@2checkFourthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 2
            add dx, 1
            cmp cx, 7
            jg @@2checkFifthSpot
            cmp dx, 7
            jg @@2checkFifthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkFifthSpot
            jmp @@isInCheck
            @@2checkFifthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            add cx, 1
            add dx, 2
            cmp cx, 7
            jg @@2checkSixthSpot
            cmp dx, 7
            jg @@2checkSixthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkSixthSpot
            jmp @@isInCheck
            @@2checkSixthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 1
            add dx, 2
            cmp cx, 0
            jl @@2checkSeventhSpot
            cmp dx, 7
            jg @@2checkSeventhSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkSeventhSpot
            jmp @@isInCheck
            @@2checkSeventhSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 2
            add dx, 1
            cmp cx, 0
            jl @@2checkEighthSpot
            cmp dx, 7
            jg @@2checkEighthSpot
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@2checkEighthSpot
            jmp @@isInCheck
            @@2checkEighthSpot:
            mov cx, @@boardX
            mov dx, @@boardY
            sub cx, 2
            sub dx, 1
            cmp cx, 0
            jl @@checkForWhiteRooks
            cmp dx, 0
            jl @@checkForWhiteRooks
            push cx
            push dx
            call getNextBoardPieceValue
            cmp al, wKNIGHT
            jne @@checkForWhiteRooks
            jmp @@isInCheck
        @@checkForWhiteRooks: ; We'll check the 4 cardinal directions and if we find a rook/queen there, with no pieces intruding then we're in check 
            mov cx, @@boardX
            mov dx, @@boardY
            cmp dx, 0
            je @@2sofUp
            @@2checkUp:
                dec dx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a Black piece is there, go check the next Direction
                jl @@2sofUp
                cmp al, wROOK
				jne @@2checkQueenUp
                jmp @@isInCheck
                @@2checkQueenUp:
				cmp al, wQUEEN
				jne @@2continueUpDirection
                jmp @@isInCheck
				@@2continueUpDirection:
                cmp al, 0
                jne @@2sofUp
                cmp dx, 0
                jne @@2checkUp

            @@2sofUp:
            mov dx, @@boardY
            cmp cx, 7
            je @@2sofRight
            @@2checkRight:
                inc cx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a Black piece is there, go check the next Direction
                jl @@2sofRight
                cmp al, wROOK
				jne @@2checkQueenRight
                jmp @@isInCheck
                @@2checkQueenRight:
				cmp al, wQUEEN
				jne @@2continueRightDirection
                jmp @@isInCheck
				@@2continueRightDirection:
                cmp al, 0
                jne @@2sofRight
                cmp cx, 7
                jne @@2checkRight

            @@2sofRight:
            mov cx, @@boardX
            cmp dx, 7
            je @@2sofDown
            @@2checkDown:
                inc dx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a Black piece is there, go check the next Direction
                jl @@2sofDown
                cmp al, wROOK
				jne @@2checkQueenDown
                jmp @@isInCheck
                @@2checkQueenDown:
				cmp al, wQUEEN
				jne @@2continueDownDirection
                jmp @@isInCheck
				@@2continueDownDirection:
                cmp al, 0
                jne @@2sofDown
                cmp dx, 7
                jne @@2checkDown

            @@2sofDown:
            mov dx, @@boardY
            cmp cx, 0
            je @@checkForWhiteKing
            @@2checkLeft:
                dec cx
                push cx
                push dx
                call getNextBoardPieceValue
                cmp al, 0 ; If a Black piece is there, go check the next Direction
                jl @@checkForWhiteKing
                cmp al, wROOK
				jne @@2checkQueenLeft
                jmp @@isInCheck
                @@2checkQueenLeft:
				cmp al, wQUEEN
				jne @@2continueLeftDirection
                jmp @@isInCheck
				@@2continueLeftDirection:
                cmp al, 0
                jne @@checkForWhiteKing
                cmp cx, 0
                jne @@2checkLeft
                
        @@checkForWhiteKing: ; We'll check a square around to see if the opponent's king is there
            mov dx, @@boardY
            dec dx
            mov bx, 3
            @@2checkLoop:
                push bx
                cmp dx, 0
                jl @@2continueLoop
                cmp dx, 7
                jg @@2continueLoop
                mov cx, @@boardX
                dec cx
                mov bx, 3
                @@2secondCheckLoop:
                    cmp cx, 0
                    jl @@2continueLoop
                    cmp cx, 7
                    jg @@2continueLoop
                    push cx
                    push dx
                    call getNextBoardPieceValue
                    cmp al, wKING
                    jne @@2continueLoop
                    pop bx
                    jmp @@isInCheck

                    @@2continueLoop:
                    inc cx
                    dec bx
                    cmp bx, 0
                    jne @@2secondCheckLoop
                inc dx
                pop bx
                dec bx
                cmp bx, 0
                jne @@2checkLoop
                
        ; We checked every possible piece that can threaten the king, if none of them can eat him then he's not in check.
        jmp @@isNotInCheck

    @@isInCheck:
        stc
        jmp @@sof
    @@isNotInCheck:
        clc
    @@sof:

    pop dx
    pop cx
    pop ax
    pop bp
    ret 4
endp isInCheck

proc copyBoardState
    push bp
    mov bp, sp
    push ax
    push cx
    push di
    push si

    mov si, offset board
    mov di, offset nextBoard
    mov cx, 64 ; Number of bytes to copy / size of board
    @@copyLoop:
        mov al, [byte ptr si]
        mov [byte ptr di], al
        inc si
        inc di
        loop @@copyLoop
    
    pop si
    pop di
    pop cx
    pop ax
    pop bp
    ret
endp copyBoardState

proc movePieceInNextBoard
    @@sourceX equ [word ptr bp + 10] ; In relation to the board, not the screen!
    @@sourceY equ [word ptr bp + 8] ; In relation to the board, not the screen!
    @@destinationX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@destinationY equ [word ptr bp + 4] ; In relation to the board, not the screen!
    
    push bp
    mov bp, sp
    push ax
    push bx
    push cx
    push dx

    ; Saves the value at the board
    push @@sourceX
    push @@sourceY
    call getNextBoardPieceValue ; Saves the piece value in AL

    ; Clears the value at the source location
    push EMPTY
    push @@sourceX
    push @@sourceY
    call setNextBoardPieceValue

    ; Sets the value at the board to previously saved value
    push ax
    push @@destinationX
    push @@destinationY
    call setNextBoardPieceValue

    pop dx
    pop cx
    pop bx
    pop ax
    pop bp
    ret 8
endp movePieceInNextBoard

proc getNextBoardPieceValue
    @@boardX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@boardY equ [word ptr bp + 4] ; In relation to the board, not the screen!

    push bp
    mov bp, sp
    push bx

    mov bx, @@boardY
    imul bx, 8
    add bx, @@boardX
    xor ah, ah
    mov al, [byte ptr bx + nextBoard]

    pop bx
    pop bp
    ret 4
endp getNextBoardPieceValue

proc setNextBoardPieceValue
    @@wantedPieceValue equ [byte ptr bp + 8]
    @@boardX equ [word ptr bp + 6] ; In relation to the board, not the screen!
    @@boardY equ [word ptr bp + 4] ; In relation to the board, not the screen!

    push bp
    mov bp, sp
    push ax
    push bx

    mov bx, @@boardY
    imul bx, 8
    add bx, @@boardX
    mov al, @@wantedPieceValue
    mov [byte ptr bx + nextBoard], al

    pop bx
    pop ax
    pop bp
    ret 6
endp setNextBoardPieceValue

proc convertFromArrayToBoard
    @@arrayIndex equ [word ptr bp + 4]
    push bp
    mov bp, sp
    push ax
    push bx

    xor dx, dx
    mov ax, @@arrayIndex
    mov bx, 8
    div bx
    mov cx, dx ; CX is now the boardX
    mov dx, ax ; DX is now the boardY

    pop bx
    pop ax
    pop bp
    ret 2
endp convertFromArrayToBoard
