import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Utils {

    // Tile class. Represents the 32 bytes color data that defines a Tile.
    // Holds an array of bytes[32].
    public static class Tile {

        public byte[] data;
        
        public Tile ()
        {
            // Initialize a tile with 32 bytes (8x8 pixel tile with 4-bit color depth)
            this.data = new byte[32]; // initialized to zero by default
        }
    }

    public static class TileRenderResult {

        public final List<Tile> vram;
        public final int maxTileIndex;

        public TileRenderResult (List<Tile> vram, int maxTileIndex)
        {
            this.vram = vram;
            this.maxTileIndex = maxTileIndex;
        }
    }

    public static boolean isInteger (String value)
    {
        try {
            Integer.parseInt(value);
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    /**
     * Read and parse tab_deltas definition file.
     * @param tabDeltasFile 
     * @return An ArrayList of Integers.
     */
    public static List<Integer> readTabDeltas (String tabDeltasFile) throws IOException
    {
        String content = new String(Files.readAllBytes(Paths.get(tabDeltasFile)));
        String arrName = "tab_deltas";

        Pattern regex = Pattern.compile("const\\s+u16\\s+" + arrName + "\\s*\\[.*?\\]\\s*=\\s*\\{([^}]+)\\}", Pattern.DOTALL);
        Matcher match = regex.matcher(content);
        if (!match.find()) {
            throw new IOException("Failed to find array " + arrName + " in input file.");
        }

        String[] parts = match.group(1).split(",");
        List<Integer> tabDeltas = new ArrayList<>();

        for (String part : parts) {
            String trimmed = part.trim();
            if (!trimmed.isEmpty() && isInteger(trimmed)) {
                tabDeltas.add(Integer.parseInt(trimmed));
            }
        }

        if (tabDeltas.size() != Consts.AP * Consts.PIXEL_COLUMNS * 4) {
            throw new IOException("Invalid number of elements in tab_deltas (expected " + 
                (Consts.AP * Consts.PIXEL_COLUMNS * 4) + "): " + tabDeltas.size());
        }

        return tabDeltas;
    }

    /**
     * Read and parse the map definition file.
     * @param mapMatrixFile 
     * @return An ArrayList of Integers.
     */
    public static List<Integer> readMapMatrix (String mapMatrixFile) throws IOException
    {
        String content = new String(Files.readAllBytes(Paths.get(mapMatrixFile)));
        String[] lines = content.split("\n");
        List<Integer> mapMatrix = new ArrayList<>();

        boolean inMapDefinition = false;

        for (String line : lines) {
            if (line.contains("const u8 map[MAP_SIZE][MAP_SIZE] =")) {
                inMapDefinition = true;
                continue;
            }

            if (inMapDefinition) {
                if (line.contains("};")) {
                    break; // End of map definition
                }

                String cleanedLine = line.trim()
                    .replaceAll("^\\{", "") // Remove opening brace
                    .replaceAll("},?$", ""); // Remove closing brace and optional comma
                
                String[] numbers = cleanedLine.split(",");
                for (String num : numbers) {
                    String trimmed = num.trim();
                    if (!trimmed.isEmpty()) {
                        try {
                            mapMatrix.add(Integer.parseInt(trimmed));
                        } catch (NumberFormatException e) {
                            // Skip invalid numbers
                        }
                    }
                }
            }
        }

        if (mapMatrix.size() != (Consts.MAP_SIZE * Consts.MAP_SIZE)) {
            throw new IOException("Invalid map_matrix dimensions (expected " + 
                (Consts.MAP_SIZE * Consts.MAP_SIZE) + ")");
        }

        return mapMatrix;
    }

    public static List<Integer> generateTabColor_d8_1 ()
    {
        List<Integer> result = new ArrayList<>(Consts.FP * (Consts.STEP_COUNT + 1));
        for (int sideDist = 0; sideDist < (Consts.FP * (Consts.STEP_COUNT + 1)); ++sideDist) {
            int d = 7 - Math.min(7, Math.floorDiv(sideDist, Consts.FP)); // the bigger the distant the darker the color is
            result.add(1 + d * 8);
        }
        return result;
    }

    /**
     * Generates tab_wall_div array.
     * @return An ArrayList of Integers.
     */
    public static List<Integer> generateTabWallDiv ()
    {
        // Vertical height calculation starts at the center
        final int WALL_H2 = (Consts.VERTICAL_ROWS * 8) / 2;

        // Additional column height modification. Positive value increases drawing column height. Negative value decreases it.
        final int ADDITIONAL_WALL_HEIGHT_MODIF = 4;

        // Initialize the tab_wall_div array
        List<Integer> tabWallDiv = new ArrayList<>(Consts.FP * (Consts.STEP_COUNT + 1));

        // Populate the tab_wall_div array
        for (int i = 0; i < tabWallDiv.size(); i++) {
            int v = (Consts.TILEMAP_COLUMNS * Consts.FP) / (i + 1);
            int div = Math.round(Math.min(v, Consts.MAX_U8));
            if ((div + ADDITIONAL_WALL_HEIGHT_MODIF) >= WALL_H2) {
                tabWallDiv.add(0);
            } else {
                tabWallDiv.add(WALL_H2 - div - ADDITIONAL_WALL_HEIGHT_MODIF);
            }
        }

        return tabWallDiv;
    }

    /**
     * Generates all the tiles originally used by the render.
     * @return An object containing the array of Tile objects, and the max tile index calculated.
     */
    public static TileRenderResult renderLoadTiles ()
    {
        // Create an array to store the tiles (vram)
        List<Tile> vram = new ArrayList<>();
        // Keep track of max index used
        int maxTileIndex = 0;

        // Create initial zero tile (height 0)
        Tile zeroTile = new Tile();
        vram.add(zeroTile);

        // ORDERING: tiles are created in groups of 8 tiles except first one which is the empty tile.
        // Each group goes from Highest to Smallest, and then each group goes from Darkest to Brightest.
        // 4 most right columns of pixels of each tile is left empty because this way the Plane B can be displaced 4 bits to the right.
        
        // Two passes: first with colors 0-7, second with colors 0 and 8-14.
        for (int pass = 0; pass < 2; pass++) {

            // 8 tiles per set
            for (int t = 1; t <= 8; t++) {
                Tile tile = new Tile();
                // 8 colors: they match with those from SGDK's ramp palettes (palette_grey, red, green, blue) first 8 colors going from darker to lighter
                for (int c = 0; c < 8; c++) {
                    // Visit the height of each tile in current set. Height here is 0 based.
                    for (int h = t - 1; h < 8; h++) {
                        // Visit the columns of current row. 1 byte holds 2 colors as per Tile definition (4 bits per color),
                        // so lower byte is color c and higher byte is color c+1, letting the RCA video signal do the blending.
                        // We only fill most left 4 columns of pixels of the tile, leaving the other 4 columns with color 0. 
                        for (int b = 0; b < 2; b++) {
                            // Determine lower and higher color values
                            int colorLow = c;
                            int colorHigh = (c + 1) == 8 ? c : c + 1;

                            // Adjust colors for second pass
                            if (pass == 1) {
                                colorLow = c == 0 ? 0 : c + 7;
                                // We clamp to color 7 since starting at SGDK's palette 8th color they repeat
                                colorHigh += 7;
                            }

                            // Combine lower and higher color bits
                            byte color = (byte)(colorLow | (colorHigh << 4));

                            // Set the color bytes in the tile
                            tile.data[4 * h + b] = color;
                        }
                    }

                    // Calculate the tile index and store the tile
                    int tileIndex = t + c * 8 + (pass * (8 * 8));
                    if (tileIndex >= vram.size()) {
                        vram.add(tile);
                    } else {
                        vram.set(tileIndex, tile);
                    }

                    if (tileIndex > maxTileIndex) {
                        maxTileIndex = tileIndex;
                    }
                }
            }
        }

        return new TileRenderResult(vram, maxTileIndex);
    }

    public static void cleanFramebuffer (int[] framebuffer)
    {
        for (int i = 0; i < Consts.VERTICAL_ROWS * Consts.TILEMAP_COLUMNS; ++i) {
            framebuffer[i] = 0; // tile 0 with all attributes in 0
        }
    }

    public static void writeVline (int h2, int tileAttrib, int[] framebuffer, int column)
    {
        // Draw a solid vertical line
        if (h2 == 0) {
            for (int y = 0; y < Consts.VERTICAL_ROWS * Consts.TILEMAP_COLUMNS; y += Consts.TILEMAP_COLUMNS) {
                framebuffer[y + column] = tileAttrib;
            }
            return;
        }

        int ta = Math.floorDiv(h2, 8); // vertical tilemap entry position
        // top tilemap entry
        framebuffer[ta * Consts.TILEMAP_COLUMNS + column] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
        // bottom tilemap entry (with flipped attribute)
        framebuffer[((Consts.VERTICAL_ROWS - 1) - ta) * Consts.TILEMAP_COLUMNS + column] = 
            (tileAttrib + (h2 & 7)) | Consts.TILE_ATTR_VFLIP_MASK;

        // Version for VERTICAL_ROWS = 24
        // Set tileAttrib which points to a colored tile.
        switch (ta) {
            case 0:     framebuffer[1 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[22 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 1:     framebuffer[2 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[21 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 2:     framebuffer[3 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[20 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 3:     framebuffer[4 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[19 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 4:     framebuffer[5 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[18 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 5:     framebuffer[6 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[17 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 6:     framebuffer[7 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[16 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 7:     framebuffer[8 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[15 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 8:     framebuffer[9 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[14 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 9:     framebuffer[10 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[13 * Consts.TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 10:    framebuffer[11 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[12 * Consts.TILEMAP_COLUMNS + column] = tileAttrib;
                        break;
        }
    }

    /**
     * Convert 16-bit unsigned value to a signed number.
     * @param value 
     * @return signed number.
     */
    public static int toSignedFrom16b (int value)
    {
        return value >= 32768 ? value - 65536 : value;
    }

    /**
     * Keeps the lowest 16 bits.
     * @param value 
     * @return 16-bit unsigned.
     */
    public static int toUnsigned16Bit (int value)
    {
        return value & 0xFFFF;
    }

    /**
     * Get the sign of a 16-bit integer.
     * @param num 
     * @return -1 if bit 16th is set, 1 if not.
     */
    public static int get16BitSign (int num)
    {
        // Check if the most significant bit is set (bit 15)
        return (num & 0x8000) != 0 ? -1 : 1;
    }
}