;---------------------------------------
; RAM
;---------------------------------------
0x0:                    ; field 00  #not used
0x1:                    ; field 01  #not used
0x2:                    ; field 02  #not used
0x3:                    ; field 03  #not used
0x4:                    ; field 40  #not used
0x5:                    ; field 41  #not used
0x6:                    ; field 42  #not used
0x7:                    ; field 43  #not used
0x8:    0000    0000    ; COL
0x9:    0000    0000    ; ROW
0xA:    0000    0001    ; in-/decrement by 1
0xB:                    ; lives
0xC:                    ; wanted
0xD:                    ; random (counted value)
0xE:    0000    0111    ; field size
0xF:    0000    0000    ; INPUTS
;---------------------------------------
; Page 0 — Display + RAM Initialization
;---------------------------------------
000 0:0     51      LDI     0001
001 0:1     E0      OUT     0000    ; Clears Display and sets DDRAM addr 0: 00 0000 0001
002 0:2     9f      LDR     1111
003 0:3     E0      OUT     0000    ; Set Display to 8bit, 2lines, 5x8 font
004 0:4     5F      LDI     1111
005 0:5     E0      OUT     0000    ; Sets Display on, Cursor on and Blink on: 00 0000 1111
006 0:6     50      LDI     0000   
007 0:7     48      STA     1000    ; COL=0 (RAM: 0x8)
008 0:8     49      STA     1001    ; ROW=0 (RAM: 0x9)
009 0:9     51      LDI     0001
010 0:A     4A      STA     1010    ; in-/decrement by 1 (RAM: 0xA)
011 0:B     57      LDI     0111
012 0:C     4E      STA     1110    ; init field size (RAM: 0xE)
013 0:D     AE      JF      1110
014 0:E     D0      1101    0000    ; Jump to Intro Screen                     
015 0:F     38      0011    1100    ; const for display function set   
;---------------------------------------
; Page 1 — Main Loop
;---------------------------------------
016 1:0     DF      IN      1111    ; Write pressed button into A and RAM at 0xf
017 1:1     CF      CMR     1111          
018 1:2     80      JZ      0000    ; if NO_INPUT            
019 1:3     A4      JF      0100        
020 1:4     20      0010    0000    ; jump to UP/DOWN Handler  
021 1:5     9E      LDR     1110
022 1:6     2D      ADD     1101
023 1:7     2D      ADD     1101
024 1:8     4C      STA     1100    ; init symbols starting point (RAM: 0xC)
025 1:9     9D      LDR     1101
026 1:A     4B      STA     1011    ; init lives (RAM: 0xB)
027 1:B     AC      JF      1100
028 1:C     80      1000    0000    ; jump to Draw Field
029 1:D     33      0011    0011    ; const for 3 lives
030 1:E     61      0110    0001    ; const for symbols starting point (a)
031 1:F     00      0000    0000    ; const mask for NO_INPUT    
;---------------------------------------
; Page 2 — Up/Down Handler
;---------------------------------------
032 2:0     CF      CMR     1111    ; check for UP
033 2:1     86      JZ      0110
034 2:2     CE      CMR     1110    ; check for DOWN
035 2:3     8A      JZ      1010
036 2:4     A5      JF      0101
037 2:5     30      0011    0000    ; back to main loop/next handler
038 2:6     50      LDI     0000 
039 2:7     49      STA     1001    ; UP
040 2:8     A9      JF      1001        
041 2:9     63      0110    0011    ; Jump to UPDATE_CURSOR UP
042 2:A     54      LDI     0100 
043 2:B     49      STA     1001    ; DOWN 
044 2:C     AD      JF      1101         
045 2:D     65      0110    0101    ; Jump to UPDATE_CURSOR DOWN
046 2:E     02      0000    0010    ; const mask for DOWN
047 2:F     01      0000    0001    ; const mask for UP
;---------------------------------------
; Page 3 — Left/Right Handler
;---------------------------------------
048 3:0     CF      CMR     1111    ; check for LEFT
049 3:1     86      JZ      0110
050 3:2     CE      CMR     1110    ; check for RIGHT
051 3:3     89      JZ      1001
052 3:4     A5      JF      0101
053 3:5     40      0100    0000    ; back to main loop/next handler
054 3:6     18      LDA     1000                          
055 3:7     3A      SUB     1010    ; decrement COL by 1
056 3:8     6B      JMP     1011
057 3:9     18      LDA     1000
058 3:A     2A      ADD     1010    ; increment COL by 1
059 3:B     48      STA     1000
060 3:C     AD      JF      1101
061 3:D     90      1001    0000    ; Jump to Boundaries Check
062 3:E     08      0000    1000    ; const mask for RIGHT
063 3:F     04      0000    0100    ; const mask for LEFT
;---------------------------------------
; Page 4 — A/(B) Handler
;---------------------------------------
064 4:0     CF      CMR     1111    ; check for A
065 4:1     84      JZ      0100
066 4:2     A3      JF      0011
067 4:3     50      0101    0000    ; back to main loop/next handler
068 4:4     18      LDA     1000
069 4:5     29      ADD     1001
070 4:6     BD      CMP     1101
071 4:7     8D      JZ      1101    ; if hit
072 4:8     1B      LDA     1011
073 4:9     3A      SUB     1010                 
074 4:A     4B      STA     1011
075 4:B     AC      JF      1100
076 4:C     B4      1011    0100    ; jump if no hit 
077 4:D     AE      JF      1110
078 4:E     B0      1011    0000    ; jump if hit
079 4:F     10      0001    0000    ; const mask for A
;---------------------------------------
; Page 5 — START/SELECT Handler
;---------------------------------------
080 5:0     CF      CMR     1111    ; check for START
081 5:1     86      JZ      0110
082 5:2     CE      CMR     1110    ; check for SELECT
083 5:3     88      JZ      1000
084 5:4     A5      JF      0101
085 5:5     10      0001    0000    ; back to main loop
086 5:6     A7      JF      0111
087 5:7     A7      1010    0111    ; jump to Reset
088 5:8     DF      IN      1111     
089 5:9     CE      CMR     1110
090 5:A     84      JZ      0100    
091 5:B     68      JMP     1000    ; PAUSE loop
092 5:C     00        
093 5:D     00                      
094 5:E     80      1000    0000    ; const mask for SELECT
095 5:F     40      0100    0000    ; const mask for START
;---------------------------------------
; Page 6 — UPDATE CURSOR
;---------------------------------------
096 6:0     54      LDI     0100
097 6:1     B9      CMP     1001    ; if row = 4
098 6:2     85      JZ      0101
099 6:3     9F      LDR     1111    ; UP
100 6:4     66      JMP     0110
101 6:5     9E      LDR     1110    ; DOWN   
102 6:6     28      ADD     1000
103 6:7     E0      OUT     0000
104 6:8     A9      JF      1001 
105 6:9     10      0001    0000    ; back to main loop 
106 6:A     9F      LDR     1111
107 6:B     E0      OUT     0000 
108 6:C     AD      JF      1101
109 6:D     15      0001    0101    ; jump to main init
110 6:E     A8      1010    1000    ; const for LCD row 1 cell 0 (40)
111 6:F     80      1000    0000    ; const for LCD row 0 cell 0 (00)
;---------------------------------------
; Page 7 — Draw HUD
;---------------------------------------
112 7:0     9f      LDR     1111
113 7:1     E0      OUT     0000
114 7:2     1C      LDA     1100    ; wanted
115 7:3     E2      OUT     0010      
116 7:4     9E      LDR     1110
117 7:5     E0      OUT     0000
118 7:6     1B      LDA     1011    ; lives
119 7:7     E2      OUT     0010
120 7:8     9D      LDR     1101
121 7:9     E0      OUT     0000
122 7:A     AB      JF      1011        
123 7:B     10      0001    0000    ; jump to main
124 7:C     00  
125 7:D     80      1000    0000    ; const for LCD row 0 cell 0 (00)
126 7:E     AD      1010    1101    ; const for LCD row 1 cell 5 (45)
127 7:F     85      1000    0101    ; const for LCD row 0 cell 5 (05)
;---------------------------------------
; Page 8 — Draw Field
;---------------------------------------
128 8:0     9F      LDR     1111
129 8:1     E2      OUT     0010
130 8:2     E2      OUT     0010
131 8:3     E2      OUT     0010
132 8:4     E2      OUT     0010
133 8:5     9E      LDR     1110
134 8:6     E0      OUT     0000        
135 8:7     9F      LDR     1111
136 8:8     E2      OUT     0010
137 8:9     E2      OUT     0010
138 8:A     E2      OUT     0010
139 8:B     E2      OUT     0010
140 8:C     AD      JF      1101
141 8:D     70      0111    0000    ; jump to Draw HUD
142 8:E     A8      1010    1000    ; const for LCD row 1 cell 0 (40)
143 8:F     58      0101    1000    ; const for hidden field (X)
;---------------------------------------
; Page 9 — Boundaries Check
;---------------------------------------
144 9:0     18      LDA     1000
145 9:1     CF      CMR     1111
146 9:2     87      JZ      0111
147 9:3     CE      CMR     1110
148 9:4     8A      JZ      1010
149 9:5     A6      JF      0110
150 9:6     60      0110    0000    ; jump to UPDATE_CURSOR
151 9:7     50      LDI     0000
152 9:8     48      STA     1000
153 9:9     65      JMP     0101
154 9:A     53      LDI     0011
155 9:B     48      STA     1000
156 9:C     65      JMP     0101
157 9:D     00
158 9:E     04      0000    0100    ; const for 4
159 9:F     FF      1111    1111    ; const for -1
;---------------------------------------
; Page 10 — Check Lives & Reset
;---------------------------------------
160 A:0     1B      LDA     1011 
161 A:1     CF      CMR     1111
162 A:2     85      JZ      0101    ; if lives = 0
163 A:3     A4      JF      0100
164 A:4     60      0110    0000    ; jump to UPDATE CURSOR
165 A:5     A6      JF      0110
166 A:6     F0      1111    0000    ; jump to Loss Screen
167 A:7     9E      LDR     1110 
168 A:8     E0      OUT     0000    ; set cell 47
169 A:9     50      LDI     0000      
170 A:A     E2      OUT     0010      
171 A:B     AC      JF      1100      
172 A:C     6A      0110    1010    ; jump to UPDATE CURSOR init cell 00 
173 A:D     00
174 A:E     AF      1010    1111    ; const for LCD row 1 cell 7 (47)  
175 A:F     30      0011    0000    ; const for 3 lives
;---------------------------------------
; Page 11 — Hit/No Hit
;---------------------------------------
176 B:0     1C      LDA     1100            
177 B:1     EA      OUT     1010  
178 B:2     A3      JF      0011  
179 B:3     E0      1110    0000    ; Jump to Win Screen       
180 B:4     9F      LDR     1111     
181 B:5     28      ADD     1000
182 B:6     29      ADD     1001
183 B:7     E6      OUT     0110      
184 B:8     9E      LDR     1110  
185 B:9     E0      OUT     0000       
186 B:A     1B      LDA     1011       
187 B:B     E2      OUT     0010       
188 B:C     AD      JF      1101        
189 B:D     A0      1010    0000    ; Jump to Check Lives                       
190 B:E     AD      1010    1101    ; const for LCD row 1 cell 5 (45)
191 B:F     70      0111    0000    ; const for symbols starting point (p)
;---------------------------------------
; Page 12 — Randomize / Start Loop
;---------------------------------------
192 C:0     DF      IN      1111
193 C:1     CF      CMR     1111
194 C:2     8C      JZ      1100    ; if START is pressed
195 C:3     1D      LDA     1101    ; random (RAM: 0xD)
196 C:4     BE      CMP     1110    
197 C:5     89      JZ      1001    ; if random = 7
198 C:6     2A      ADD     1010
199 C:7     4D      STA     1101    ; increment by 1
200 C:8     60      JMP     0000    ; loop
201 C:9     50      LDI     0000
202 C:A     4D      STA     1101
203 C:B     60      JMP     0000    ; loop
204 C:C     AD      JF      1101
205 C:D     A7      1010    0111    ; jump to Reset
206 C:E     00
207 C:F     40      0100    0000    ; const for START
;---------------------------------------
; Page 13 — Intro Screen
;---------------------------------------
208 D:0     9F      LDR     1111    
209 D:1     E6      OUT     0110    ; F
210 D:2     9E      LDR     1110      
211 D:3     EA      OUT     1010    ; 1
212 D:4     9D      LDR     1101                    
213 D:5     E2      OUT     0010    ; N
214 D:6     9C      LDR     1100                  
215 D:7     E2      OUT     0010    ; D
216 D:8     A9      JF      1001
217 D:9     C0      1100    0000    ; jump to Randomize 
218 D:A     00
219 D:B     00      
220 D:C     44      0100    0100    ; const for D
221 D:D     4E      0100    1110    ; const for N
222 D:E     31      0011    0001    ; const for 1
223 D:F     46      0100    0110    ; const for F
;---------------------------------------
; Page 14 — Win Screen
;---------------------------------------
224 E:0     9E      LDR     1110
225 E:1     E8      OUT     1000    ; set cell 47
226 E:2     9F      LDR     1111      
227 E:3     EA      OUT     1010    ; W
228 E:4     50      LDI     0000
229 E:5     48      STA     1000    ; COL=0 (RAM: 0x8)
230 E:6     49      STA     1001    ; ROW=0 (RAM: 0x9)     
231 E:7     A8      JF      1000
232 E:8     C0      1100    0000    ; jump to Randomize 
233 E:9     00
234 E:A     00
235 E:B     00
236 E:C     00
237 E:D     00
238 E:E     AF      1010    1111    ; const for LCD row 1 cell 7 (47) 
239 E:F     57      0101    0111    ; const for W
;---------------------------------------
; Page 15 — Loss Screen
;---------------------------------------
240 F:0     9E      LDR     1100 
241 F:1     E8      OUT     0100    ; set cell 47
242 F:2     9F      LDR     1111  
243 F:3     E6      OUT     0110    ; L
244 F:4     50      LDI     0000 
245 F:5     48      STA     1000    ; COL=0 (RAM: 0x8)
246 F:6     49      STA     1001    ; ROW=0 (RAM: 0x9)  
247 F:7     A8      JF      1000
248 F:8     C0      1100    0000    ; jump to Randomize
249 F:9     00
250 F:A     00 
251 F:B     00
252 F:C     00
253 F:D     00
254 F:E     AF      1010    1111    ; const for LCD row 1 cell 7 (47)  
255 F:F     4C      0100    1100    ; const for L
