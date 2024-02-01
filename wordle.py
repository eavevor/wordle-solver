import random
import pygame
import pygame.locals
import wordle_solver

SQUARE_SIZE = 80
PADDING = 15

SIZE = PADDING + 5 * (SQUARE_SIZE +  PADDING), PADDING + 6 * (SQUARE_SIZE + PADDING)
YELLOW = (181,159,59)
GRAY = (58,58,60)
GREEN = (83,141,78)
COLORS = [GRAY, YELLOW, GREEN]

def convert_color(c):
    if (c == COLORS.index(GRAY)):
        return 'K'
    if (c == COLORS.index(YELLOW)):
        return 'Y'
    if (c == COLORS.index(GREEN)):
        return 'G'

    
pygame.init()
rects = []
colors = [0 for i in range(30)]
screen = pygame.display.set_mode(SIZE)
characters = ['' for i in range(30)]
for j in range(6):
    for i in range(5):
        rects.append(pygame.locals.Rect(PADDING + i * (SQUARE_SIZE + PADDING), PADDING + j * (SQUARE_SIZE + PADDING), SQUARE_SIZE, SQUARE_SIZE))

running = True
pygame.font.init()
pygame.display.set_caption('Wordle Solver')
my_font = pygame.font.Font(pygame.font.get_default_font(), SQUARE_SIZE * 3 // 5)
character_idx = 0
word_list = [line.strip() for line in open('word-list.txt', 'r').readlines() if len(line.strip())]

while running:
    for event in pygame.event.get():
        if event.type == pygame.locals.QUIT:
            running = False

        if event.type == pygame.MOUSEBUTTONUP:
            pos = pygame.mouse.get_pos()
            # x =  padding / 2 + i * (width + padding)
            # i = (x - padding/2) / (width + padding)
            i = (pos[0] - PADDING // 2) // (SQUARE_SIZE + PADDING)
            j = (pos[1] - PADDING // 2) // (SQUARE_SIZE + PADDING)
            if 0 <= i < 5 and 0 <= j < 6:
                idx = i + j * 5
                colors[idx] = (colors[idx] + 1) % 3

        if event.type == pygame.KEYUP:
            if event.key == pygame.K_SPACE:
                colors[character_idx - 1] = (colors[character_idx - 1] + 1) % 3
            if event.key == pygame.K_BACKSPACE:
                character_idx-=1
                characters[character_idx] = ''
                colors[character_idx] = COLORS.index(GRAY)
                character_idx = max(0, character_idx)
            elif event.key == pygame.K_RETURN:
                words = [w for w in [''.join(characters[i: i + 5]) for i in range(0, 30, 5)] if len(w) == 5]
                if len(words) == 0:
                    print("Enter a starting word. I suggest starting with 'storm'")
                    continue
                convert_colors = lambda x: ''.join([convert_color(t) for t in x])
                word_colors = [convert_colors(colors[i: i + 5]) for i in range(0, 30, 5)][:len(words)]
                word_list_clone = word_list.copy()
                random.shuffle(word_list_clone)
                for _w, _c in zip(words, word_colors):
                    # wittle down the word list based on the guesses made
                    word_list_clone = wordle_solver.filter_words(word_list_clone, _w, _c)
                if len(word_list_clone) == 0:
                    print("Word not in the answer list")
                    break
                length, optimum = wordle_solver.get_optimal_word(word_list_clone)
                print(f"The game is solvable in {length - 1} or fewer subsequent guesses using {optimum}. {len(word_list_clone)} guesses left.")
                # automatically add the optimum to the screen
                character_idx = (character_idx // 5) * 5
                for i in range(5):
                    characters[character_idx] = optimum[i]
                    # show colours as green where appropriate
                    if (all(word[i] == optimum[i] for word in word_list_clone)):
                        colors[character_idx] = COLORS.index(GREEN)
                    character_idx+=1 
            else:
                c = pygame.key.name(event.key)
                if(len(c) == 1 and ord('A') <= ord(c.upper()) <= ord('Z')):
                    characters[character_idx] = c
                    character_idx+=1


    screen.fill((18,18,19))
    for rect, color in zip(rects, colors):
        pygame.draw.rect(screen, COLORS[color], rect)
    for i, character in enumerate(characters):
        text_surface = my_font.render(character.upper(), True, (255, 255, 255))
        w, h = text_surface.get_width(), text_surface.get_height()
        x,y = i % 5, i // 5
        x, y = PADDING + x * (SQUARE_SIZE + PADDING),  PADDING + y * (SQUARE_SIZE + PADDING)
        x += (SQUARE_SIZE - w)//2
        y += (SQUARE_SIZE - h)//2

        screen.blit(text_surface, (x, y))
    pygame.display.update()


pygame.quit()