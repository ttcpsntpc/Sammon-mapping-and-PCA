# Operations
WASD for camera and light, you can switch them by pressing Q
IJKL for camera rotate
# Read voxel
## Sol. 1
1. raed .inf and get object resolution
2. create corresponding voxel three dimension array (vector)  
index is position  
the value is distance field
3. read .raw into array
4. draw voxel
## Sol. 2
1. read inf and raw file
2. create a large cube that can accommodate the model
3. create a 3D texture for cube
    * create a 4D array -> ``3D_texture[resolution.x][resolution.y][resolution.z][4]``
    * 4 means gradiant on x/y/z and the intensity (硬度值)
    * use this 4D array create 3D texture
4. create a 2D array that connects the intensity and color -> ``color_map[255][4]``
    * 255 means the internsity value scope
    * 4 means RGBA
5. modify fragment shader
# Distance Field
## Variables
1. read model and set model voxel as 0
```c 
double distance[100][100] // set model voxel 0
int voxelStatus[100][100]; // record DONE, CLOSE, FAR
```
2. compute distance
```c
double closeHeap[n];
vec2 heapIndex[n];
```