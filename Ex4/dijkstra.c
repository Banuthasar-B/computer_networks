#include <stdio.h>

#define MAX 20
#define INF 9999

/* --- ANSI Terminal Color Configurations --- */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

struct Node
{
    char name;
    int dist;
    int parent;
    int visited;
};

struct Node n[MAX];
int graph[MAX][MAX];

/* ---------------- Simple Array-Based Min Heap ---------------- */
int heapArr[MAX];
int heapPos[MAX];
int heapSize;

void swapHeap(int i, int j)
{
    int temp;
    temp = heapArr[i];
    heapArr[i] = heapArr[j];
    heapArr[j] = temp;
    heapPos[heapArr[i]] = i;
    heapPos[heapArr[j]] = j;
}

void heapifyDown(int i)
{
    int smallest, left, right;
    while (1)
    {
        smallest = i;
        left = 2 * i + 1;
        right = 2 * i + 2;
        
        if (left < heapSize && n[heapArr[left]].dist < n[heapArr[smallest]].dist)
        {
            smallest = left;
        }
        if (right < heapSize && n[heapArr[right]].dist < n[heapArr[smallest]].dist)
        {
            smallest = right;
        }
        if (smallest == i)
        {
            break;
        }
        swapHeap(i, smallest);
        i = smallest;
    }
}

void heapifyUp(int i)
{
    int parent;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (n[heapArr[i]].dist < n[heapArr[parent]].dist)
        {
            swapHeap(i, parent);
            i = parent;
        }
        else
        {
            break;
        }
    }
}

void buildHeap(int vertices)
{
    int i;
    heapSize = vertices;
    for (i = 0; i < vertices; i++)
    {
        heapArr[i] = i;
        heapPos[i] = i;
    }
    for (i = vertices / 2 - 1; i >= 0; i--)
    {
        heapifyDown(i);
    }
}

int extractMin()
{
    int minVertex;
    if (heapSize == 0)
    {
        return -1;
    }
    minVertex = heapArr[0];
    heapArr[0] = heapArr[heapSize - 1];
    heapPos[heapArr[0]] = 0;
    heapSize--;
    heapifyDown(0);
    return minVertex;
}

void decreaseKey(int vertex)
{
    int i;
    i = heapPos[vertex];
    heapifyUp(i);
}

/* ---------------- Dijkstra Logic & Simplified Layouts ---------------- */
void initialize(int vertices, int src)
{
    int i;
    for (i = 0; i < vertices; i++)
    {
        n[i].dist = INF;
        n[i].parent = -1;
        n[i].visited = 0;
    }
    n[src].dist = 0;
}

void printCurrentTable(int vertices)
{
    int i;
    printf("\n  Current Routing Status:\n");
    printf("  %-10s %-10s %-12s\n", "Router", "Cost", "Came From");
    for (i = 0; i < vertices; i++)
    {
        if (n[i].dist == INF)
        {
            printf("  %-10c " RED "%-10s" RESET " %-12s\n", n[i].name, "INF", "-");
        }
        else if (n[i].parent == -1)
        {
            printf("  " GREEN "%-10c" RESET " " GREEN "%-10d" RESET " " BOLD "%-12s" RESET "\n", n[i].name, n[i].dist, "START");
        }
        else
        {
            printf("  %-10c " YELLOW "%-10d" RESET " %-12c\n", n[i].name, n[i].dist, n[n[i].parent].name);
        }
    }
    printf("\n");
}

void dijkstra(int vertices, int src)
{
    int u, j, oldDist, newDist, stepCount;
    initialize(vertices, src);
    buildHeap(vertices);
    
    printf("\n" BOLD GREEN "--- DIJKSTRA LOG TRACKER ---" RESET "\n");
    
    stepCount = 0;
    while (heapSize > 0)
    {
        u = extractMin();
        if (n[u].dist == INF)
        {
            break;
        }
        n[u].visited = 1;
        stepCount++;
        
        printf("\n" MAGENTA "Step %d:" RESET " Visiting '" BOLD "%c" RESET "' (Cost: %d)\n",
               stepCount, n[u].name, n[u].dist);
               
        for (j = 0; j < vertices; j++)
        {
            if (graph[u][j] != 0 && n[j].visited == 0)
            {
                oldDist = n[j].dist;
                newDist = n[u].dist + graph[u][j];
                
                if (newDist < n[j].dist)
                {
                    n[j].dist = newDist;
                    n[j].parent = u;
                    decreaseKey(j);
                    
                    if (oldDist == INF)
                    {
                        printf("  -> Path found to '%c' via %c (Cost: %d)\n", n[j].name, n[u].name, newDist);
                    }
                    else
                    {
                        printf("  -> Cheaper path to '%c' found: %d -> %d\n", n[j].name, oldDist, newDist);
                    }
                }
            }
        }
        printCurrentTable(vertices);
    }
}

char pathBuf[120];
int pathLen;

void buildPath(int vertex)
{
    if (n[vertex].parent == -1)
    {
        pathBuf[pathLen] = n[vertex].name;
        pathLen++;
        return;
    }
    buildPath(n[vertex].parent);
    
    pathBuf[pathLen++] = ' ';
    pathBuf[pathLen++] = '-';
    pathBuf[pathLen++] = '>';
    pathBuf[pathLen++] = ' ';
    pathBuf[pathLen++] = n[vertex].name;
}

int getFirstHop(int dest, int src)
{
    int cur = dest;
    while (n[cur].parent != src)
    {
        cur = n[cur].parent;
    }
    return cur;
}

void printFinalTable(int vertices, int src)
{
    int i, firstHop, pathWidth;
    pathWidth = vertices * 5 + 3;
    if (pathWidth < 12)
    {
        pathWidth = 12;
    }
    
    printf("\n" BOLD YELLOW "--- FINAL COMPLETED ROUTING MAP (Source: %c) ---" RESET "\n\n", n[src].name);
    printf("  %-8s %-8s %-12s %-12s %-*s\n",
           "Router", "Cost", "First Hop", "Direct Via", pathWidth, "Full Route Path");
    
    for (i = 0; i < vertices; i++)
    {
        if (n[i].dist == INF)
        {
            printf("  %-8c " RED "%-8s" RESET " %-12s %-12s " RED "%-*s" RESET "\n",
                   n[i].name, "INF", "-", "-", pathWidth, "Not reachable");
        }
        else if (i == src)
        {
            pathLen = 0;
            buildPath(i);
            pathBuf[pathLen] = '\0';
            printf("  " BOLD "%-8c" RESET " " GREEN "%-8d" RESET " %-12s %-12s " CYAN "%-*s" RESET "\n",
                   n[i].name, 0, "-", "-", pathWidth, pathBuf);
        }
        else
        {
            firstHop = getFirstHop(i, src);
            pathLen = 0;
            buildPath(i);
            pathBuf[pathLen] = '\0';
            printf("  %-8c " YELLOW "%-8d" RESET " %-12c %-12c %-*s\n",
                   n[i].name, n[i].dist, n[firstHop].name, n[n[i].parent].name, pathWidth, pathBuf);
        }
    }
    printf("\n");
}

int main()
{
    int vertices;
    int i, j, src;
    
    printf(BOLD WHITE "--- ROUTER SHORTEST PATH CALCULATOR ---" RESET "\n\n");
    
    printf("Enter number of routers: ");
    if (scanf("%d", &vertices) != 1) return 0;
    
    if (vertices > MAX)
    {
        printf(RED "Max routers allowed is %d\n" RESET, MAX);
        return 0;
    }
    
    printf("\nRouter Names\n");
    for (i = 0; i < vertices; i++)
    {
        printf("Name of router %d: ", i);
        if (scanf(" %c", &n[i].name) != 1) return 0;
    }
    
    printf("\nLink Costs Between Routers\n");
    for (i = 0; i < vertices; i++)
    {
        for (j = 0; j < vertices; j++)
        {
            if (i == j)
            {
                graph[i][j] = 0;
                continue;
            }
            if (j < i)
            {
                graph[i][j] = graph[j][i];
                continue;
            }
            printf("Cost from %c to %c (0 = no direct link): ", n[i].name, n[j].name);
            if (scanf("%d", &graph[i][j]) != 1) return 0;
            
            if (graph[i][j] < 0)
            {
                printf(RED "Negative cost is not allowed.\n" RESET);
                return 0;
            }
        }
    }
    
    printf("\nSource Selection\n");
    printf("Enter source router index (0 to %d): ", vertices - 1);
    if (scanf("%d", &src) != 1) return 0;
    
    if (src < 0 || src >= vertices)
    {
        printf(RED "Invalid source router configuration\n" RESET);
        return 0;
    }
    
    dijkstra(vertices, src);
    printFinalTable(vertices, src);
    
    return 0;
}

