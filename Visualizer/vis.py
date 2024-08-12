import pygame
import os

# Initialize Pygame
pygame.init()

# Set up the display
WIDTH, HEIGHT = 800, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Dice Shape Comparison")

# Colors
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
COLORS = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0), (255, 0, 255)]

def read_obj(filename):
    points = []
    edges = set()
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('v '):
                _, x, y, z = line.split()
                points.append([float(x), float(y), float(z)])
            elif line.startswith('f '):
                face = [int(v.split('/')[0]) - 1 for v in line.split()[1:]]
                for i in range(len(face)):
                    edge = tuple(sorted((face[i], face[(i+1) % len(face)])))
                    edges.add(edge)
    return points, list(edges)

def read_shape(filename):
    if filename.endswith('.obj'):
        return read_obj(filename)
    with open(filename, 'r') as f:
        lines = f.readlines()
        num_points = int(lines[0])
        points = [list(map(float, line.strip().split(','))) for line in lines[1:num_points+1]]
        num_edges = int(lines[num_points+1])
        edges = [tuple(sorted(map(int, line.strip().split(',')))) for line in lines[num_points+2:]]
    return points, edges

def write_shape(filename, points, edges):
    with open(filename, 'w') as f:
        f.write(f"{len(points)}\n")
        for point in points:
            f.write(f"{point[0]:.2f},{point[1]:.2f},{point[2]:.2f}\n")
        count = 0
        for edge in edges:
            if edge[0] != edge[1]:
                count += 1
        f.write(f"{count}\n")
        for edge in edges:
            if edge[0] != edge[1]:
                f.write(f"{edge[0]},{edge[1]}\n")

def write_obj(filename, points, edges):
    with open(filename, 'w') as f:
        for point in points:
            f.write(f"v {point[0]:.6f} {point[1]:.6f} {point[2]:.6f}\n")
        
        # Remove duplicate edges and sort each edge
        edges = set(tuple(sorted(edge)) for edge in edges)
        
        # Create faces from edges
        faces = []
        edge_set = set(map(tuple, edges))
        while edge_set:
            face = []
            start_edge = edge_set.pop()
            current_vertex = start_edge[1]
            face.append(start_edge[0])
            face.append(current_vertex)
            
            while True:
                next_edge = next((e for e in edge_set if e[0] == current_vertex), None)
                if next_edge:
                    edge_set.remove(next_edge)
                    current_vertex = next_edge[1]
                    face.append(current_vertex)
                else:
                    break
            
            if len(face) > 2:
                faces.append(face)
        
        for face in faces:
            f.write(f"f {' '.join(str(v+1) for v in face)}\n")


def project(point, scale, offset):
    x, y, z = point
    return (int(x * scale + offset[0]), int(y * scale + offset[1]))

# Load shapes
shapes = {}
for i in range(4, 21):
    if os.path.exists(f"{i}"):
        shapes[i] = read_shape(f"{i}")
    elif os.path.exists(f"{i}.obj"):
        shapes[i] = read_shape(f"{i}.obj")

# Set initial scales
scales = {i: 1.0 for i in shapes}

# Main loop
running = True
selected_shape = None
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                running = False
            elif event.key in [pygame.K_4, pygame.K_5, pygame.K_6, pygame.K_7, pygame.K_8, pygame.K_9, 
                               pygame.K_0, pygame.K_1, pygame.K_2, pygame.K_3]:
                shape_num = event.key - pygame.K_0 if event.key != pygame.K_0 else 10
                if shape_num == 1:
                    shape_num = 12
                if shape_num > 10:
                    shape_num = 10 + shape_num % 10
                if shape_num in shapes:
                    selected_shape = shape_num
            elif event.key == pygame.K_UP and selected_shape:
                scales[selected_shape] *= 1.1
            elif event.key == pygame.K_DOWN and selected_shape:
                scales[selected_shape] /= 1.1
            elif event.key == pygame.K_s:
                for i, (points, edges) in shapes.items():
                    scaled_points = [[p[0]*scales[i], p[1]*scales[i], p[2]*scales[i]] for p in points]
                    write_shape(f"{i}", scaled_points, edges)
                print("All shapes saved with current scales.")
            elif event.key == pygame.K_o:
                for i, (points, edges) in shapes.items():
                    scaled_points = [[p[0]*scales[i], p[1]*scales[i], p[2]*scales[i]] for p in points]
                    write_obj(f"{i}.obj", scaled_points, edges)
                print("All shapes exported as OBJ files with current scales.")

    screen.fill(BLACK)

    # Draw shapes
    for idx, (shape_num, (points, edges)) in enumerate(shapes.items()):
        scale = scales[shape_num] * 2
        offset = ((idx % 3 + 1) * WIDTH // 4, (idx // 3 + 1) * HEIGHT // 4)
        
        color = COLORS[idx % len(COLORS)]
        
        # Draw edges
        for edge in edges:
            start = project(points[edge[0]], scale, offset)
            end = project(points[edge[1]], scale, offset)
            pygame.draw.line(screen, color, start, end)
        
        # Draw shape number
        font = pygame.font.Font(None, 36)
        text = font.render(str(shape_num), True, WHITE)
        screen.blit(text, (offset[0] - 20, offset[1] - 20))

    # Draw instructions
    font = pygame.font.Font(None, 24)
    instructions = [
        "Press number keys to select a shape",
        "Up/Down arrows to resize selected shape",
        "Press 'S' to save all shapes",
        "Press 'O' to export as OBJ",
        "ESC to quit"
    ]
    for i, instruction in enumerate(instructions):
        text = font.render(instruction, True, WHITE)
        screen.blit(text, (10, 10 + i * 30))

    pygame.display.flip()

pygame.quit()
