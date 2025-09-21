import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Execute in Windows with:
 * javac -cp . Tiles_Pair_Generator.java Utils.java Consts.java && java -cp . Tiles_Pair_Generator && del /f Tiles_Pair_Generator*.class Utils*.class Consts*.class
 */
public class Tiles_Pair_Generator {

    private static final String tabDeltasFile = "../inc/tab_deltas.h";
    private static final String mapMatrixFile = "../src/map_matrix.c";
    private static final String outputFile = "tiles_pair_OUTPUT.txt";
    private static final int TILE_INDEX_MASK = 0x7FF;
    private static final int MAX_JOBS = 256;
    
    // Progress tracking
    private static final AtomicInteger completedIterations = new AtomicInteger(0);
    private static final long totalIterations = (long) (Consts.MAX_POS_XY - Consts.MIN_POS_XY + 1) * 
            (Consts.MAX_POS_XY - Consts.MIN_POS_XY + 1) * (1024 / (1024 / Consts.AP));

    // Use thread-local storage for frequently used objects
    private static ThreadLocal<int[]> framebufferPlaneA = ThreadLocal.withInitial(() -> 
        new int[Consts.VERTICAL_ROWS * Consts.TILEMAP_COLUMNS]);
    private static ThreadLocal<int[]> framebufferPlaneB = ThreadLocal.withInitial(() -> 
        new int[Consts.VERTICAL_ROWS * Consts.TILEMAP_COLUMNS]);
    private static ThreadLocal<StringBuilder> keyBuilder = ThreadLocal.withInitial(StringBuilder::new);

    private static String createTrackingKey(int i, int j) {
        StringBuilder sb = keyBuilder.get();
        sb.setLength(0);
        return sb.append(i).append('-').append(j).toString();
    }

    private static void trackTilePairsBetweenPlanes(int[] framebufferPlaneA, int[] framebufferPlaneB, Map<String, Integer> tilePairMap) {
        for (int i = 0; i < Consts.VERTICAL_ROWS * Consts.TILEMAP_COLUMNS; i++) {
            // Construct the key using the bitmask TILE_INDEX_MASK which only keeps the tile index data
            String key = createTrackingKey(framebufferPlaneA[i] & TILE_INDEX_MASK, framebufferPlaneB[i] & TILE_INDEX_MASK);
            tilePairMap.put(key, 1);
        }
    }

    // Processing function to be run inside the worker thread
    private static Map<String, Integer> processGameChunk(String jobId, int startPosX, int endPosX, 
            List<Integer> tabDeltas, List<Integer> tabWallDiv, List<Integer> tabColorD81, List<Integer> map) {

        int[] localFramebufferPlaneA = framebufferPlaneA.get();
        int[] localFramebufferPlaneB = framebufferPlaneB.get();
        Map<String, Integer> tilePairMap = new HashMap<>(2048);
        final int posStepping = 1;

        // NOTE: here we are moving from the most UPPER-LEFT position of the map[][] layout, 
        // stepping DOWN into Y Axis, and RIGHT into X Axis, where in each position we do a full rotation.
        // Therefore we only interesting in collisions with x+1 and y+1.

        for (int posX = startPosX; posX <= endPosX; posX += posStepping) {
            for (int posY = Consts.MIN_POS_XY; posY <= Consts.MAX_POS_XY; posY += posStepping) {

                // Current location normalized
                int x = Math.floorDiv(posX, Consts.FP);
                int y = Math.floorDiv(posY, Consts.FP);

                // Limit Y axis location normalized
                int ytop = Math.floorDiv((posY - (Consts.MAP_FRACTION - 1)), Consts.FP);
                int ybottom = Math.floorDiv((posY + (Consts.MAP_FRACTION - 1)), Consts.FP);

                // Check X axis collision
                // Moving right as per map[][] layout?
                if (map.get(y * Consts.MAP_SIZE + (x + 1)) != 0 || 
                    map.get(ytop * Consts.MAP_SIZE + (x + 1)) != 0 || 
                    map.get(ybottom * Consts.MAP_SIZE + (x + 1)) != 0) {
                    
                    if (posX > ((x + 1) * Consts.FP - Consts.MAP_FRACTION)) {
                        // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units
                        posX += (Consts.FP + 2 * Consts.MAP_FRACTION) - posStepping;
                        // Update progress
                        completedIterations.addAndGet((Consts.MAX_POS_XY - posY + 1) * (1024 / (1024 / Consts.AP)));
                        // Stop current Y and continue with next X until it gets outside the collision
                        break;
                    }
                }
                // Moving left as per map[][] layout?
                // else if (map.get(y*Consts.MAP_SIZE + (x-1)).intValue() != 0 || 
                //         map.get(ytop*Consts.MAP_SIZE + (x-1)).intValue() != 0 || 
                //         map.get(ybottom*Consts.MAP_SIZE + (x-1)).intValue() != 0) {
                //     if (posX < (x*Consts.FP + Consts.MAP_FRACTION)) {
                //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
                //         posX -= (Consts.FP + 2*Consts.MAP_FRACTION) + posStepping;
                //         // Send progress update to main thread
                //         completedIterations.addAndGet((Consts.MAX_POS_XY - posY + 1) * (1024 / (1024 / Consts.AP)));
                //         // Stop current Y and continue with next X until it gets outside the collision
                //         break;
                //     }
                // }

                // Limit X axis location normalized
                int xleft = Math.floorDiv((posX - (Consts.MAP_FRACTION - 1)), Consts.FP);
                int xright = Math.floorDiv((posX + (Consts.MAP_FRACTION - 1)), Consts.FP);

                // Check Y axis collision
                // Moving down as per map[][] layout?
                if (map.get((y + 1) * Consts.MAP_SIZE + x).intValue() != 0 || 
                    map.get((y + 1) * Consts.MAP_SIZE + xleft).intValue() != 0 || 
                    map.get((y + 1) * Consts.MAP_SIZE + xright).intValue() != 0) {
                    
                    if (posY > ((y + 1) * Consts.FP - Consts.MAP_FRACTION)) {
                        // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units
                        posY += (Consts.FP + 2 * Consts.MAP_FRACTION) - posStepping;
                        // Update progress
                        completedIterations.addAndGet(1024 / (1024 / Consts.AP));
                        // Continue with next Y until it gets outside the collision
                        continue;
                    }
                }
                // Moving up as per map[][] layout?
                // else if (map.get((y-1)*Consts.MAP_SIZE + x).intValue() != 0 || 
                //         map.get((y-1)*Consts.MAP_SIZE + xleft).intValue() != 0 || 
                //         map.get((y-1)*Consts.MAP_SIZE + xright).intValue() != 0) {
                //     if (posY < (y*Consts.FP + Consts.MAP_FRACTION)) {
                //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
                //         posY -= (Consts.FP + 2*Consts.MAP_FRACTION) + posStepping;
                //         // Send progress update to main thread
                //         completedIterations.addAndGet(1024 / (1024 / Consts.AP));
                //         // Continue with next Y until it gets outside the collision
                //         continue;
                //     }
                // }

                for (int angle = 0; angle < 1024; angle += (1024 / Consts.AP)) {
                    Utils.cleanFramebuffer(localFramebufferPlaneA);
                    Utils.cleanFramebuffer(localFramebufferPlaneB);

                    // DDA (Digital Differential Analyzer)

                    int sideDistX_l0 = posX - (Math.floorDiv(posX, Consts.FP) * Consts.FP);
                    int sideDistX_l1 = (Math.floorDiv(posX, Consts.FP) + 1) * Consts.FP - posX;
                    int sideDistY_l0 = posY - (Math.floorDiv(posY, Consts.FP) * Consts.FP);
                    int sideDistY_l1 = (Math.floorDiv(posY, Consts.FP) + 1) * Consts.FP - posY;

                    int a = Math.floorDiv(angle, (1024 / Consts.AP));

                    for (int column = 0; column < Consts.PIXEL_COLUMNS; ++column) {
                        // instead of: const u16 *delta_a_ptr = tab_deltas + (a * PIXEL_COLUMNS * 4);
                        // we use the offset directly when accessing tab_deltas
                        int baseIndex = (a * Consts.PIXEL_COLUMNS * 4) + column * 4;

                        int deltaDistX = tabDeltas.get(baseIndex + 0);
                        int deltaDistY = tabDeltas.get(baseIndex + 1);
                        int rayDirX = Utils.toSignedFrom16b(tabDeltas.get(baseIndex + 2));
                        int rayDirY = Utils.toSignedFrom16b(tabDeltas.get(baseIndex + 3));

                        int sideDistX, stepX;

                        if (rayDirX < 0) {
                            stepX = -1;
                            sideDistX = Utils.toUnsigned16Bit((sideDistX_l0 * deltaDistX) >> Consts.FS);
                        } else {
                            stepX = 1;
                            sideDistX = Utils.toUnsigned16Bit((sideDistX_l1 * deltaDistX) >> Consts.FS);
                        }

                        int sideDistY, stepY, stepYMS;

                        if (rayDirY < 0) {
                            stepY = -1;
                            stepYMS = -Consts.MAP_SIZE;
                            sideDistY = Utils.toUnsigned16Bit((sideDistY_l0 * deltaDistY) >> Consts.FS);
                        } else {
                            stepY = 1;
                            stepYMS = Consts.MAP_SIZE;
                            sideDistY = Utils.toUnsigned16Bit((sideDistY_l1 * deltaDistY) >> Consts.FS);
                        }

                        int mapX = Math.floorDiv(posX, Consts.FP);
                        int mapY = Math.floorDiv(posY, Consts.FP);

                        for (int n = 0; n < Consts.STEP_COUNT_LOOP; ++n) {
                            // side X
                            if (sideDistX < sideDistY) {
                                mapX += stepX;
                                int hit = map.get(mapY * Consts.MAP_SIZE + mapX);
                                if (hit != 0) {
                                    int h2 = tabWallDiv.get(sideDistX); // height halved
                                    int d8_1 = tabColorD81.get(sideDistX); // the bigger the distant the darker the color is
                                    int tileAttrib;
                                    
                                    if ((mapY & 1) != 0) {
                                        tileAttrib = (Consts.PAL0 << Consts.TILE_ATTR_PALETTE_SFT) | (d8_1 + (8 * 8));
                                    } else {
                                        tileAttrib = (Consts.PAL0 << Consts.TILE_ATTR_PALETTE_SFT) | d8_1;
                                    }

                                    if ((column % 2) == 0) {
                                        Utils.writeVline(h2, tileAttrib, localFramebufferPlaneA, column / 2);
                                    } else {
                                        Utils.writeVline(h2, tileAttrib, localFramebufferPlaneB, column / 2);
                                    }
                                    break;
                                }
                                sideDistX += deltaDistX;
                            }
                            // side Y
                            else {
                                mapY += stepY;
                                int hit = map.get(mapY * Consts.MAP_SIZE + mapX);
                                if (hit != 0) {
                                    int h2 = tabWallDiv.get(sideDistY); // height halved
                                    int d8_1 = tabColorD81.get(sideDistY); // the bigger the distant the darker the color is
                                    int tileAttrib;
                                    
                                    if ((mapX & 1) != 0) {
                                        tileAttrib = (Consts.PAL1 << Consts.TILE_ATTR_PALETTE_SFT) + d8_1 + (8 * 8);
                                    } else {
                                        tileAttrib = (Consts.PAL1 << Consts.TILE_ATTR_PALETTE_SFT) + d8_1;
                                    }

                                    if ((column % 2) == 0) {
                                        Utils.writeVline(h2, tileAttrib, localFramebufferPlaneA, column / 2);
                                    } else {
                                        Utils.writeVline(h2, tileAttrib, localFramebufferPlaneB, column / 2);
                                    }
                                    break;
                                }
                                sideDistY += deltaDistY;
                            }
                        }
                    }

                    // Traverse both framebuffers and track generated pairs between each entry
                    trackTilePairsBetweenPlanes(localFramebufferPlaneA, localFramebufferPlaneB, tilePairMap);
                }

                // Update progress
                completedIterations.addAndGet(1024 / (1024 / Consts.AP));
            }
        }

        return tilePairMap;
    }

    private static void writeOutputToFile(Map<String, Integer> finalMap, String outputFile) throws IOException {
        List<String> mapEntries = new ArrayList<>(finalMap.keySet());

        // Sort the entries by key (numberA-numberB)
        mapEntries.sort((a, b) -> {
            // Split the keys into numberA and numberB
            String[] aParts = a.split("-");
            String[] bParts = b.split("-");
            
            int aNumA = Integer.parseInt(aParts[0]);
            int aNumB = Integer.parseInt(aParts[1]);
            int bNumA = Integer.parseInt(bParts[0]);
            int bNumB = Integer.parseInt(bParts[1]);

            // Compare numberA first
            if (aNumA != bNumA) {
                return Integer.compare(aNumA, bNumA); // Sort by numberA ascending
            }
            // If numberA is the same, compare numberB
            return Integer.compare(aNumB, bNumB); // Sort by numberB ascending
        });

        StringBuilder outputString = new StringBuilder();
        for (String key : mapEntries) {
            outputString.append(key).append("\n"); // Format: "key"
        }

        Files.write(Paths.get(outputFile), outputString.toString().getBytes());
    }

    private static void displayProgress() {
        double progress = (completedIterations.get() / (double) totalIterations) * 100;
        int progressBars = (int) (progress / 2);
        StringBuilder progressBar = new StringBuilder();
        for (int i = 0; i < progressBars; i++) progressBar.append("=");
        for (int i = progressBars; i < 50; i++) progressBar.append("-");
        
        System.out.printf("\r[%s] %.2f%%", progressBar.toString(), progress);
    }

    // Function to run parallel workers
    public static void runWorkers() throws IOException, InterruptedException, ExecutionException {
        int numCores = Math.min(16, Runtime.getRuntime().availableProcessors());
        List<Integer> tabDeltas = Utils.readTabDeltas(tabDeltasFile);
        List<Integer> tabWallDiv = Utils.generateTabWallDiv();
        List<Integer> tabColorD81 = Utils.generateTabColor_d8_1();
        List<Integer> mapMatrix = Utils.readMapMatrix(mapMatrixFile);
        
        ExecutorService executor = Executors.newFixedThreadPool(numCores);
        List<Future<Map<String, Integer>>> futures = new ArrayList<>();

        System.out.println("Utilizing " + numCores + " core/s for processing multiple jobs.");
        String startTimeStr = new Date().toString();
        System.out.println(startTimeStr);

        // Create job queue
        List<Job> jobQueue = new ArrayList<>();
        int chunkSize = (int) Math.ceil((double) (Consts.MAX_POS_XY - Consts.MIN_POS_XY + 1) / MAX_JOBS);
        for (int startPosX = Consts.MIN_POS_XY; startPosX <= Consts.MAX_POS_XY; startPosX += chunkSize) {
            int endPosX = Math.min(startPosX + chunkSize - 1, Consts.MAX_POS_XY);
            jobQueue.add(new Job(startPosX, endPosX));
        }

        // Reset iterations counter
        completedIterations.set(0);

        int totalJobs = jobQueue.size();
        System.out.println("Main thread: Total jobs to complete: " + totalJobs);
        System.out.println("Main thread: Waiting for all workers to complete their jobs...");

        // Submit all jobs to executor
        for (Job job : jobQueue) {
            futures.add(executor.submit(() -> {
                return processGameChunk(job.toString(), job.startPosX, job.endPosX, tabDeltas, tabWallDiv, tabColorD81, mapMatrix);
            }));
        }

        // Start progress display thread
        Thread progressThread = new Thread(() -> {
            while (!executor.isTerminated()) {
                displayProgress();
                try {
                    Thread.sleep(4000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        });
        progressThread.start();

        // Wait for all tasks to complete
        Map<String, Integer> finalMap = new HashMap<>();
        for (Future<Map<String, Integer>> future : futures) {
            Map<String, Integer> result = future.get();
            finalMap.putAll(result);
        }

        executor.shutdown();
        progressThread.interrupt();
        progressThread.join();

        displayProgress();
        System.out.println(); // Move to the next line after progress bar

        // Write output to file
        writeOutputToFile(finalMap, outputFile);
        System.out.println("Processing complete. Output saved to tiles_pair_OUTPUT.txt");
        String endTimeStr = new Date().toString();
        System.out.println(endTimeStr);
    }

    public static void main(String[] args) {
        try {
            runWorkers();
        } catch (Exception e) {
            System.err.println("[ERROR] " + e.getMessage());
            e.printStackTrace();
            String endTimeStr = new Date().toString();
            System.out.println(endTimeStr);
        }
    }

    private static class Job {
        public final int startPosX;
        public final int endPosX;

        public Job(int startPosX, int endPosX) {
            this.startPosX = startPosX;
            this.endPosX = endPosX;
        }

        @Override
        public String toString() {
            return "[" + startPosX + "-" + endPosX + "]";
        }
    }
}