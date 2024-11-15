import sys, math, pygame
from operator import itemgetter
import copy

def parser(string) -> tuple:
    x_s, y_s, z_s = string.split(',')[2:5]
    return float(x_s), float(y_s), float(z_s)
    # print(string.split(',')[2:5])
    # print(f"X: {float(x_s)} | Y: {float(y_s)} | Z: {float(z_s)}")



class Point3D:
    def __init__(self, x=0, y=0, z=0):
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)

    def rotateX(self, angle):
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        y = self.y * cosa - self.z * sina
        z = self.y * sina + self.z * cosa

        return Point3D(self.x, y, z)
    
    
    def rotateY(self, angle):
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        z = self.z * cosa - self.x * sina
        x = self.z * sina + self.x * cosa
        
        return Point3D(x, self.y, z)
    

    def rotateZ(self, angle):
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        x = self.x * cosa - self.y * sina
        y = self.x * sina + self.y * cosa
        
        return Point3D(x, y, self.z)
    

    def project(self, win_width, win_height, fov, viewer_distance):
        factor = fov / (viewer_distance + self.z)
        x = self.x * factor + win_width / 2
        y = -self.y * factor + win_height / 2

        return Point3D(x, y, 1)
    
class Simulation:
    
    def __init__(self, win_width=640, win_height=480, filter_width=15):
        self.x_filter = []
        self.y_filter = []
        self.z_filter = []
        self.filter_width = filter_width
        self.num_elements = 0
        
        pygame.init()

        self.screen = pygame.display.set_mode((win_width, win_height))
        pygame.display.set_caption("3D Wireframe Cube Simulation")

        self.clock = pygame.time.Clock() 

        self.verticies = [
            Point3D(-1, 1, -1),
            Point3D(1, 1, -1),
            Point3D(1, -1, -1),
            Point3D(-1, -1, -1),
            Point3D(-1, 1, 1),
            Point3D(1, 1, 1),
            Point3D(1, -1, 1),
            Point3D(-1, -1, 1),
        ]

        self.faces = [(0,1,2,3), (1,5,6,2), (5,4,7,6), (4,0,3,7), (0,4,5,1), (3,2,6,7)]

        self.colors = [(255,0,255), (255,0,0),(0,255,0),(0,0,255),(0,255,255),(255,255,0)]

        self.angleX, self.angleY, self.angleZ = 0, 0, 0

    def run(self, data):
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
        
        self.clock.tick(50)
        self.screen.fill((0,0,0))

        t = []
        for v in self.verticies:
            r = v.rotateX(self.angleX).rotateY(self.angleY).rotateZ(self.angleZ)

            p = r.project(self.screen.get_width(), self.screen.get_height(), 256, 4)

            t.append(p)

        avg_z = []
        i = 0
        for f in self.faces:
            z = (t[f[0]].z + t[f[1]].z + t[f[3]].z) / 4
            avg_z.append([i, z])
            i += 1
        
        for tmp in sorted(avg_z, key=itemgetter(1), reverse=True):
            face_index = tmp[0]
            f = self.faces[face_index]

            pointlist = [(t[f[0]].x, t[f[0]].y), (t[f[1]].x, t[f[1]].y),
                            (t[f[1]].x, t[f[1]].y), (t[f[2]].x, t[f[2]].y),
                            (t[f[2]].x, t[f[2]].y), (t[f[3]].x, t[f[3]].y),
                            (t[f[3]].x, t[f[3]].y), (t[f[0]].x, t[f[0]].y),]
            pygame.draw.polygon(self.screen, self.colors[face_index], pointlist)

        # Here i need to parse the data

        self.angleX, self.angleX, self.angleZ = parser(data)
        self.x_filter.append(self.angleX)
        self.y_filter.append(self.angleY)
        self.z_filter.append(self.angleZ)
        
        self.angleX = sum(self.x_filter) / len(self.x_filter)
        self.angleY = sum(self.y_filter) / len(self.y_filter)
        self.angleZ = sum(self.z_filter) / len(self.z_filter)

        if len(self.x_filter) > self.filter_width:
            self.x_filter.pop(0)
            self.y_filter.pop(0)
            self.z_filter.pop(0)
        # print(len(self.x_filter))
        
        pygame.display.flip()
                