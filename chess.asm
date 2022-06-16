IDEAL
MODEL small
STACK 100h
LOCALS
p386

include 'GraphAss.inc'
include 'LogicAss.inc'

DATASEG
; -------------------------
; This is used to track which player can do a move
playerTurn db 1 ; 1 will mean it's White's turn, -1 will mean it's Black's turn

; We put these names in the "board" variable, which we use to differentiate the piece
; (and due to jumps of 2 we can do use the values in a lookup table)
wKING equ 12
wQUEEN equ 10
wROOK equ 8
wKNIGHT equ 6
wBISHOP equ 4
wPAWN equ 2

EMPTY equ 0

bPAWN equ -2
bBISHOP equ -4
bKNIGHT equ -6
bROOK equ -8
bQUEEN equ -10
bKING equ -12

board 	db bROOK, bKNIGHT, bBISHOP, bQUEEN, bKING, bBISHOP, bKNIGHT, bROOK ; 0
		db bPAWN, bPAWN, bPAWN, bPAWN, bPAWN, bPAWN, bPAWN, bPAWN ; 1
		db EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY ; 2
		db EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY ; 3
		db EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY ; 4
		db EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY ; 5
		db wPAWN, wPAWN, wPAWN, wPAWN, wPAWN, wPAWN, wPAWN, wPAWN ; 6
		db wROOK, wKNIGHT, wBISHOP, wQUEEN, wKING, wBISHOP, wKNIGHT, wROOK ; 7

highlighterX dw 0 ; These will be used to keep track of the player controlled highlighter
highlighterY dw 0 ; (which is used to show the player which square he's selecting)

highlighterConfirmedX dw -1 ; These will be used to save the source square which is decided upon ENTER
highlighterConfirmedY dw -1 ; We'll set them as impossible values so we can check if we already got a source

; Basic victory states messages
whiteWonMessage db 'White won!', '$'
blackWonMessage db 'Black won!', '$'
noOneWonMessage db 'No one won!', '$'

; Start menu messages
welcomeMessage db 'Welcome to CHESS', 0
jokeMessage db 'May I take your order?', 0
startButton db 'Start', 0
; --------------------------
CODESEG

; --------------------------
; Purpose:
; This is called when we want a player to do an action (whether it be moving the highlighter or selecting a source/destination)
; --------------------------
; How:
; Checks whether the keyboard buffer is empty or not, if it is then the Z flag will turn on and we'll repeat the check
; when we do have something in the buffer, we check if it's one of the allowed keys (Arrows, ENTER, ESC)
; If it is one of them it'll continue to the main program, else it'll check again for a keypress
;
; Parameters: None
;
; Returns: The key pressed in AH = scan code, AL = ASCII value
; --------------------------
proc waitForInput
    @@start:
    mov ah, 1h
    int 16h ; Checks whether a keystroke is in the buffer
    jz @@start ; If there isn't keystroke then check again
    mov ah, 0h
    int 16h ; Puts the keystroke into AL

	; We'll check if the player pressed any of the keys we use in the game, if not then check again
    cmp al, 27
    je @@sof ; If it isn't ESC then continue to check the mouse, else go to the program's draw result
    ; We'll check if ENTER is pressed because that is our selection confirm key
    cmp al, 0Dh ; ENTER
    je @@sof
    ; Now we'll check if any of the Arrow keys were pressed (we'll check their Scan codes since they don't have ASCII)
    cmp ah, 48h ; Up Arrow
    je @@sof
    cmp ah, 50h ; Down Arrow
    je @@sof
    cmp ah, 4Dh ; Right Arrow
    je @@sof
    cmp ah, 4Bh ; Left Arrow
    jne @@start ; If it isn't equal to any of them then check again

    @@sof:
    ret
endp waitForInput

; playSound - Plays a single tone
; CX = tone, BX = duration
proc playSound
    mov     al, 182
    out     43h, al
    mov     ax, cx

    out     42h, al
    mov     al, ah
    out     42h, al
    in      al, 61h

    or      al, 00000011b
    out     61h, al

    @@pause1:
        mov cx, 65535

    @@pause2:
        dec cx
        jne @@pause2
        dec bx
        jne @@pause1

        in  al, 61h
        and al, 11111100b
        out 61h, al
    ret
endp playSound

start:
	mov ax, @data
	mov ds, ax
	; Initialize Graphic mode memory
	mov ax, 0a000h
    mov es, ax
	; Enter Graphics mode
	mov ax, 13h
	int 10h
; --------------------------
; Your code here
call startGameIntro
call randomizeTheme
call drawBoard
call initializeBoard
call indicatePlayerTurn

push highlightColor
push 0
push 0
call drawhighlight

gameLoop:
	call waitForInput
	cmp al, 27
	je tie
	cmp al, 0Dh ; Check if it was ENTER
	je confirmPressed
    cmp ah, 48h ; Check the Up Arrow
    je moveHighlighterUp
    cmp ah, 50h ; Check the Down Arrow
    je moveHighlighterDown
    cmp ah, 4Dh ; Check the Right Arrow
    je moveHighlighterRight
    cmp ah, 4Bh ; Check the Left Arrow
    je moveHighlighterLeft
	jmp gameLoop ; This isn't supposed to be reached, but I put it here just in case ;P

	moveHighlighterUp:
		cmp [highlighterY], 0 ; Check whether the Y is at the maximum coordinate, if it is then we don't want to add to it
		je gameLoop
		; We need to check we're not drawing over the confirmed highlighter
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@isOk
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je @@moveHighlighter
		@@isOk:
		; We'll fix the previous square by moving the piece to the same spot, which redraws the square and the piece
		push [highlighterX]
		push [highlighterY]
		push [highlighterX]
		push [highlighterY]
		call movePiece
		@@moveHighlighter:
		; Check if we're at the confirmed highlighter if we aren't then draw the highlighter
		sub [highlighterY], 1
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@sof
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je gameLoop
		@@sof:
		; And now we can draw the highlighter
		push highlightColor
		push [highlighterX]
		push [highlighterY]
		call drawHighlight
		jmp gameLoop

	moveHighlighterDown:
		cmp [highlighterY], 7 ; Check whether the Y is at the minimum coordinate, if it is then we don't want to add to it
		je gameLoop
		; We need to check we're not deleting the confirmed highlighter
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@isOk
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je @@moveHighlighter
		@@isOk:
		; We'll fix it by moving the piece to the same spot, which redraws the square and the piece
		push [highlighterX]
		push [highlighterY]
		push [highlighterX]
		push [highlighterY]
		call movePiece
		@@moveHighlighter:
		; Check if we're at the confirmed highlighter if we aren't then draw the highlighter
		add [highlighterY], 1
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@sof
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je gameLoop
		@@sof:
		; And now we can draw the highlighter
		push highlightColor
		push [highlighterX]
		push [highlighterY]
		call drawHighlight
		jmp gameLoop

	moveHighlighterRight:
		cmp [highlighterX], 7 ; Check whether the X is at the maximum coordinate, if it is then we don't want to add to it
		je gameLoop
		; We need to check we're not drawing over the confirmed highlighter
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@isOk
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je @@moveHighlighter
		@@isOk:
		; We'll fix it by moving the piece to the same spot, which redraws the square and the piece
		push [highlighterX]
		push [highlighterY]
		push [highlighterX]
		push [highlighterY]
		call movePiece
		@@moveHighlighter:
		; Check if we're at the confirmed highlighter if we aren't then draw the highlighter
		add [highlighterX], 1
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@sof
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je gameLoop
		@@sof:
		; And now we can draw the highlighter
		push highlightColor
		push [highlighterX]
		push [highlighterY]
		call drawHighlight
		jmp gameLoop

	moveHighlighterLeft:
		cmp [highlighterX], 0 ; Check whether the Y is at the maximum coordinate, if it is then we don't want to add to it
		je gameLoop
		; We need to check we're not drawing over the confirmed highlighter
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@isOk
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je @@moveHighlighter
		@@isOk:
		; We'll fix it by moving the piece to the same spot, which redraws the square and the piece
		push [highlighterX]
		push [highlighterY]
		push [highlighterX]
		push [highlighterY]
		call movePiece
		@@moveHighlighter:
		; Check if we're at the confirmed highlighter if we aren't then draw the highlighter
		sub [highlighterX], 1
		mov ax, [highlighterX]
		cmp ax, [highlighterConfirmedX]
		jne @@sof
		mov ax, [highlighterY]
		cmp ax, [highlighterConfirmedY]
		je gameLoop
		@@sof:
		; And now we can draw the highlighter
		push highlightColor
		push [highlighterX]
		push [highlighterY]
		call drawHighlight
		jmp gameLoop

	confirmPressed:
		cmp [highlighterConfirmedX], -1
		jne secondConfirm
		cmp [highlighterConfirmedY], -1
		jne secondConfirm

		firstConfirm:
			mov ax, [highlighterX]
			mov [highlighterConfirmedX], ax
			mov ax, [highlighterY]
			mov [highlighterConfirmedY], ax
			
			push confirmedHighlightColor
			push [highlighterConfirmedX]
			push [highlighterConfirmedY]
			call drawHighlight
			jmp gameLoop
		
		secondConfirm:	
			@@sourceCheck:
				push [highlighterConfirmedX] ; We'll check whether the player picked an empty spot or his opponents' piece
				push [highlighterConfirmedY]
				call checkSourcePick
			jc @@invalidMove ; If the CARRY flag is turned on then the source is invalid

			@@destinationCheck:
				push [highlighterX] ; We'll check whether the player tried to go to his own piece
				push [highlighterY]
				call checkDestinationPick
			jc @@invalidMove ; If the CARRY flag is turned on then the destination is invalid

			@@validateMove:
				push [highlighterConfirmedX] ; We'll check whether the move attempted is legal (piece can move like that)
				push [highlighterConfirmedY]
				push [highlighterX]
				push [highlighterY]
				call validateMove
			jc @@invalidMove ; If the CARRY flag is turned on then the move is illegal
			
			@@checkForCheck:
				call copyBoardState ; We'll check if after the move is done the king is placed in check
				push [highlighterConfirmedX]
				push [highlighterConfirmedY]
				push [highlighterX]
				push [highlighterY]
				call movePieceInNextBoard
				push [word playerturn]
				call getKingPosition
				push cx
				push dx
				call isInCheck
			jc @@invalidMove ; If the CARRY flag is turned on then the king is in check and the move doesn't stop check

			@@validMove:
				push [highlighterConfirmedX]
				push [highlighterConfirmedY]
				push [highlighterX]
				push [highlighterY]
				call movePiece
				push [highlighterX]
				push [highlighterY]
				call drawHighlight
				
				cmp [playerTurn], 1
				je @@playWhiteSound
				@@playBlackSound:
					mov cx, 2919
					mov bx, 16h
					call playSound
					jmp @@endSound
				@@playWhiteSound:
					mov cx, 9192
					mov bx, 16h
					call playSound
				@@endSound:
				jmp @@sof
			@@invalidMove:
				push [highlighterConfirmedX]
				push [highlighterConfirmedY]
				push [highlighterConfirmedX]
				push [highlighterConfirmedY]
				call movePiece
				
				; We'll make the highlighter red temporarily to show it's invalid
				push invalidHighlightColor
				push [highlighterX]
				push [highlighterY]
				call drawHighlight
								
				@@playInvalidSound:
					mov cx, 1234
					mov bx, 20h
					call playSound
				; Reset the value of the confirmed highlighter
				mov [highlighterConfirmedX], -1
				mov [highlighterConfirmedY], -1
				jmp gameLoop

			@@sof:
			; Reset the value of the confirmed highlighter
			mov [highlighterConfirmedX], -1
			mov [highlighterConfirmedY], -1
			jmp endTurn

	endTurn:
	call copyBoardState
	push [word playerturn]
	call checkForCheckmate
	cmp al, 0
	je noWinYet
	jg whiteWon
	jl blackWon

	noWinYet:
	xor [playerturn], 11111110b ; This switches the playerTurn between -1 to 1 and back repeatedly
	call indicatePlayerTurn
	jmp gameLoop
; --------------------------
	
	whiteWon:
		; Return to text mode  
		mov ax, 2h
		int 10h
		lea dx, [whiteWonMessage]
		mov ah, 9h
		int 21h
		jmp exit
	blackWon:
		; Return to text mode  
		mov ax, 2h
		int 10h
		lea dx, [blackWonMessage]
		mov ah, 9h
		int 21h
		jmp exit
	tie:
		; Return to text mode  
		mov ax, 2h
		int 10h
		lea dx, [noOneWonMessage]
		mov ah, 9h
		int 21h
		jmp exit
exit:
	; Exit program
	mov ax, 4c00h
	int 21h
END start