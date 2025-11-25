#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>

// ---------- Personal ID validation ----------
int isValidPersonalID(const char *personal_id) {
    int has_at = 0;
    int has_number = 0;
    int has_capital = 0;
    int length = strlen(personal_id);
    
    if (length < 5 || length > 30) {
        return 0;
    }
    
    for (int i = 0; personal_id[i]; i++) {
        if (personal_id[i] == '@') {
            has_at = 1;
        } else if (personal_id[i] >= '0' && personal_id[i] <= '9') {
            has_number = 1;
        } else if (personal_id[i] >= 'A' && personal_id[i] <= 'Z') {
            has_capital = 1;
        }
    }
    
    return (has_at && has_number && has_capital);
}

void sanitizePersonalID(char *personal_id) {
    for (int i = 0; personal_id[i]; i++) {
        if (personal_id[i] == ' ') personal_id[i] = '_';
        if (personal_id[i] == '/') personal_id[i] = '-';
    }
}

// ---------- Registration / Login structures & functions ----------
void createUserFolder() {
    mkdir("users", 0777);
}

int personalIDExists(const char *personal_id) {
    char path[200];
    sprintf(path, "users/%s.txt", personal_id);
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

void saveUserData(const char *personal_id, const char *password, int day, int month, int cycle, int *userSymptoms, int userSymptomCount, int everythingFineSelected) {
    char path[200];
    sprintf(path, "users/%s.txt", personal_id);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n%s\n%d %d %d\n", personal_id, password, day, month, cycle);
        
        // Save symptom data
        fprintf(f, "SymptomData:");
        if (everythingFineSelected) {
            fprintf(f, "13"); // 13 represents "Everything is fine"
        } else {
            for (int i = 0; i < userSymptomCount; i++) {
                fprintf(f, "%d", userSymptoms[i] + 1); // Convert back to 1-based indexing
                if (i < userSymptomCount - 1) fprintf(f, ",");
            }
        }
        fprintf(f, "\n");
        
        fclose(f);
    }
}

int loadUserData(const char *personal_id, char *password, int *day, int *month, int *cycle) {
    char path[200];
    sprintf(path, "users/%s.txt", personal_id);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char pid[100];
    fscanf(f, "%s", pid);
    fscanf(f, "%s", password);
    fscanf(f, "%d %d %d", day, month, cycle);
    // Skip the symptom data line for login purposes
    char buffer[200];
    fgets(buffer, sizeof(buffer), f); // Skip to next line
    fgets(buffer, sizeof(buffer), f); // Skip symptom data line
    fclose(f);
    return 1;
}

// ---------- Fixed function to update symptoms ----------
void updateUserSymptoms(const char *personal_id, int *userSymptoms, int userSymptomCount, int everythingFineSelected) {
    char path[200];
    sprintf(path, "users/%s.txt", personal_id);
    
    // First, read all existing data
    char personal_id_read[100], password[100];
    int day, month, cycle;
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    fscanf(f, "%s", personal_id_read);
    fscanf(f, "%s", password);
    fscanf(f, "%d %d %d", &day, &month, &cycle);  // Read day, month, cycle as separate integers
    
    fclose(f);
    
    // Now rewrite the file with updated symptoms
    f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n%s\n%d %d %d\n", personal_id_read, password, day, month, cycle);  // Write them as separate integers
        
        // Save updated symptom data
        fprintf(f, "SymptomData:");
        if (everythingFineSelected) {
            fprintf(f, "13"); // 13 represents "Everything is fine"
        } else {
            for (int i = 0; i < userSymptomCount; i++) {
                fprintf(f, "%d", userSymptoms[i] + 1); // Convert back to 1-based indexing
                if (i < userSymptomCount - 1) fprintf(f, ",");
            }
        }
        fprintf(f, "\n");
        
        fclose(f);
    }
}

// Registration function without scanf - takes all parameters as input
int registerUser(const char *personal_id, const char *password, int day, int month, int cycle, char *outPersonalID, char *outPassword, int *outDay, int *outMonth, int *outCycle) {
    createUserFolder();
    
    // Copy input parameters to output
    strcpy(outPersonalID, personal_id);
    strcpy(outPassword, password);
    *outDay = day;
    *outMonth = month;
    *outCycle = cycle;
    
    // Validate personal ID
    if (!isValidPersonalID(personal_id)) {
        return 0; // Invalid Personal ID
    }
    
    if (personalIDExists(personal_id)) {
        return -1; // Personal ID already exists
    }
    
    // Validate date
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    int currentYear = now->tm_year + 1900;
    
    int monthDays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if ((currentYear % 400 == 0) || (currentYear % 4 == 0 && currentYear % 100 != 0)) monthDays[2] = 29;
    if (month < 1 || month > 12) return -2; // Invalid month
    if (day < 1 || day > monthDays[month]) return -3; // Invalid day
    
    // Validate cycle length
    if (cycle < 21 || cycle > 35) {
        return -4; // Invalid cycle length
    }
    
    int emptySymptoms[0];
    saveUserData(personal_id, password, day, month, cycle, emptySymptoms, 0, 0);
    return 1; // Success
}

// Login function without scanf
int loginUser(const char *personal_id, const char *password, char *outPersonalID, char *outPassword, int *outDay, int *outMonth, int *outCycle) {
    char storedPassword[100];
    
    strcpy(outPersonalID, personal_id);
    
    if (!personalIDExists(personal_id)) {
        return 0; // Personal ID not registered
    }
    
    if (!loadUserData(personal_id, storedPassword, outDay, outMonth, outCycle)) {
        return -1; // Failed to load user data
    }
    
    strcpy(outPassword, storedPassword);
    
    if (strcmp(password, storedPassword) != 0) {
        return -2; // Incorrect password
    }
    
    return 1; // Success
}

// ---------- Menstrual tracking structures and functions ----------
typedef struct {
    const char *symptom;
    int low;
    int high;
} SymptomFreq;

// ---------- Helpers (date validation, diff) ----------
int isValidDate(int day, int month, int year) {
    int monthDays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if ((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0)) monthDays[2] = 29;
    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > monthDays[month]) return 0;
    return 1;
}

int isFutureDate(int d, int m, int y, int td, int tm, int ty) {
    if (y > ty) return 1;
    if (y == ty && m > tm) return 1;
    if (y == ty && m == tm && d > td) return 1;
    return 0;
}

int daysBetweenDates(struct tm start, struct tm end) {
    time_t s = mktime(&start);
    time_t e = mktime(&end);
    double diff = difftime(e, s);
    return (int)(diff / (60 * 60 * 24));
}

// ---------- Date calculation helpers ----------
void addDaysToDate(struct tm *date, int days) {
    time_t t = mktime(date);
    t += days * 24 * 60 * 60;
    *date = *localtime(&t);
}

void printFormattedDate(struct tm date) {
    char *months[] = {"January", "February", "March", "April", "May", "June",
                     "July", "August", "September", "October", "November", "December"};
    printf("%d %s %d", date.tm_mday, months[date.tm_mon], date.tm_year + 1900);
}

// ---------- Detail functions (kept for "Know more" feature) ----------
void showMenstrualDetails() {
    const char *details[] = {
        "•The period is the first phase of the menstrual cycle. Estrogen and progesterone levels drop near the end of the previous cycle if pregnancy does not occur.",
        "•During menstruation, the uterus sheds the lining called the endometrium. Old blood and tissue exit the body through the vagina.",
        "•Periods vary widely. The average bleeding time for adults is 4.5 to 6 days. Typical blood loss ranges from 5 ml to 80 ml.",
        "•Tracking your cycle helps identify patterns and improves predictions for your next cycle.",
        "References:\n1. Varney's Midwifery (2019)\n2. The Lancet Child & Adolescent Health (2018)\n3. FIGO Recommendations (2011)"
    };
    int count = sizeof(details) / sizeof(details[0]);
    printf("\n--- More Information: Menstrual Phase ---\n");
    for (int i = 0; i < count; i++) printf("\n%s\n", details[i]);
}

void showFollicularDetails() {
    const char *details[] = {
        "•The follicular phase begins right after menstruation and lasts until ovulation.",
        "•Follicle-stimulating hormone (FSH) rises, causing several follicles in the ovaries to mature. Usually, only one becomes dominant.",
        "•Estrogen levels steadily increase, improving mood, energy, focus, and skin quality.",
        "•The uterine lining (endometrium) starts to rebuild and thicken in preparation for possible pregnancy.",
        "•Exercise tolerance is higher during this phase, and strength or endurance workouts may feel easier.",
        "References:\n1. Guyton & Hall Textbook of Medical Physiology (2020)\n2. Endocrine Reviews Journal (2017)\n3. ACOG Clinical Guidelines (2019)"
    };
    int count = sizeof(details) / sizeof(details[0]);
    printf("\n--- More Information: Follicular Phase ---\n");
    for (int i = 0; i < count; i++) printf("\n%s\n", details[i]);
}

void showOvulationDetails() {
    const char *details[] = {
        "•Ovulation occurs when a mature egg is released from the dominant ovarian follicle.",
        "•This usually happens about 10-16 days before the next period, depending on the cycle length.",
        "•Luteinizing hormone (LH) surges sharply, triggering the release of the egg within 24-36 hours.",
        "•Fertility is highest during this phase. Cervical mucus becomes clear, stretchy, and slippery (egg-white consistency).",
        "•Some individuals experience mild abdominal pain called 'mittelschmerz' on one side of the pelvis.",
        "References:\n1. Williams Gynecology (2020)\n2. Human Reproduction Update Journal (2016)\n3. ACOG Fertility Awareness Guidelines (2020)"
    };
    int count = sizeof(details) / sizeof(details[0]);
    printf("\n--- More Information: Ovulation Phase ---\n");
    for (int i = 0; i < count; i++) printf("\n%s\n", details[i]);
}

void showLutealDetails() {
    const char *details[] = {
        "•The luteal phase occurs after ovulation and lasts until the next period begins.",
        "•Progesterone levels rise significantly, which can lead to symptoms known as premenstrual syndrome (PMS).",
        "•Common PMS symptoms include headaches, breast tenderness, bloating, water retention, irritability, anxiety, tearfulness, fatigue, acne, and food cravings.",
        "•Some individuals may also experience positive changes such as increased creativity, higher libido, or improved confidence.",
        "•Tracking symptoms can help identify patterns and understand what factors worsen or ease them, such as sleep, diet, and stress levels.",
        "References:\n1. ACOG - Premenstrual Syndrome (2015)\n2. Gender Medicine Journal (2012)\n3. Feminism & Psychology Journal (2013)\n4. Women & Health Journal (2003)"
    };
    int count = sizeof(details) / sizeof(details[0]);
    printf("\n--- More Information: Luteal Phase ---\n");
    for (int i = 0; i < count; i++) printf("\n%s\n", details[i]);
}

const char *fixedSymptoms[] = {
    "Cramps",
    "Tender breasts",
    "Fatigue",
    "Bloating",
    "Headache",
    "Acne",
    "Backache",
    "Nausea",
    "Cravings",
    "Insomnia",
    "Constipation",
    "Diarrhea",
    "Everything is fine"
};
int fixedSymCount = sizeof(fixedSymptoms)/sizeof(fixedSymptoms[0]);

// ---------- Symptom data for all detailed sub-ranges (updated scientifically) ----------
// Menstrual: Days 1-3, Days 4-5
SymptomFreq menstrual_1_3[] = {
    {"Cramps", 50, 90},
    {"Fatigue", 60, 80},
    {"Lower back pain", 50, 70},
    {"Diarrhea/loose stools", 30, 50},
    {"Headache", 30, 50},
    {"Bloating", 40, 60}
};
int menstrual_1_3_count = 6;

SymptomFreq menstrual_4_5[] = {
    {"Cramps (reduced)", 20, 40},
    {"Fatigue", 30, 50},
    {"Light bleeding", 100, 100},
    {"Mood improving", 10, 30}
};
int menstrual_4_5_count = 4;

// General menstrual common symptoms (very common + common)
const char *menstrual_general[] = {
    "Cramps",
    "Fatigue",
    "Lower back pain",
    "Bloating",
    "Diarrhea",
    "Headache",
    "Light bleeding towards end of this phase",
    "Improved mood towards end of this phase"
};
int menstrual_general_count = 8;

// Follicular: Days 6-8, 9-11, 12-13
SymptomFreq follicular_6_8[] = {
    {"Energy rising", 60, 90},
    {"Improved mood", 50, 80},
    {"Clearer skin", 20, 40}
};
int follicular_6_8_count = 3;

SymptomFreq follicular_9_11[] = {
    {"Slight increase in libido", 20, 40},
    {"Mild breast tenderness", 10, 20},
    {"Better cognition / focus", 30, 60}
};
int follicular_9_11_count = 3;

SymptomFreq follicular_12_13[] = {
    {"Increasing cervical mucus", 60, 90},
    {"Increased libido", 40, 70},
    {"Possible early ovulation signs", 10, 20}
};
int follicular_12_13_count = 3;

// Follicular general (very common + common)
const char *follicular_general[] = {
    "Energy rising",
    "Mood stable / improving",
    "Clearer skin",
    "Increase in libido",
    "Mild breast tenderness towards end of this phase",
    "Better cognition / focus towards end of this phase",
    "Increasing cervical mucus towards end of this phase"
};
int follicular_general_count = 7;

// Ovulation: Day 14
SymptomFreq ovulation_14[] = {
    {"Stretchy clear cervical mucus", 80, 90},
    {"Libido spike", 60, 70},
    {"Mittelschmerz (ovulation pain)", 20, 40},
    {"Mild nausea", 15, 25},
    {"Mild breast tenderness", 20, 30}
};
int ovulation_14_count = 5;

// Ovulation general
const char *ovulation_general[] = {
    "Stretchy clear cervical mucus",
    "Increased libido",
    "Mittelschmerz (ovulation pain)",
    "Mild nausea and breast tenderness towards the end of this phase"
};
int ovulation_general_count = 4;

// Luteal: Days 15-18, 19-22, 23-26, 27-28
SymptomFreq luteal_15_18[] = {
    {"Fatigue", 40, 60},
    {"Bloating", 40, 60},
    {"Breast tenderness", 60, 75}
};
int luteal_15_18_count = 3;

SymptomFreq luteal_19_22[] = {
    {"Irritability", 50, 60},
    {"Mood swings", 40, 60},
    {"Headache", 30, 50},
    {"Cravings (carbs)", 40, 60}
};
int luteal_19_22_count = 4;

SymptomFreq luteal_23_26[] = {
    {"Anxiety", 30, 50},
    {"Heightened irritability", 60, 75},
    {"Fatigue", 60, 70}
};
int luteal_23_26_count = 3;

SymptomFreq luteal_27_28[] = {
    {"Bloating", 60, 70},
    {"Headache", 40, 60},
    {"Lower back pain", 30, 50},
    {"Return of cramps (mild)", 30, 50}
};
int luteal_27_28_count = 4;

// Luteal general (very common + common)
const char *luteal_general[] = {
    "Breast tenderness",
    "Fatigue",
    "Bloating",
    "Mood swings",
    "Irritability",
    "Headache",
    "Cravings(crabs)",
    "Anxiety",
    "Lower back pain and Return of cramps(mild) towards end of this phase"
};
int luteal_general_count = 9;

// ---------- Helpers to manage printed symptoms and avoid duplicates ----------
int alreadyPrinted(const char *symptom, const char printed[][60], int printedCount) {
    for (int i = 0; i < printedCount; i++) if (strcmp(symptom, printed[i]) == 0) return 1;
    return 0;
}

int isInGeneral(const char *symptom, const char **generalList, int genCount) {
    for (int i = 0; i < genCount; i++) if (strcmp(symptom, generalList[i]) == 0) return 1;
    return 0;
}

void markPrinted(const char *symptom, char printed[][60], int *printedCount) {
    if (!alreadyPrinted(symptom, printed, *printedCount)) {
        strncpy(printed[*printedCount], symptom, 59);
        printed[*printedCount][59] = '\0';
        (*printedCount)++;
    }
}

// ---------- Mapping cycle day to sub-range index and pointers ----------
typedef struct {
    const char *label;
    SymptomFreq *arr;
    int count;
    const char **genPtr;
    int genCount;
} RangeInfo;

// We'll build a static array of ranges in order
#define RANGE_TOTAL 10
void buildRanges(RangeInfo ranges[]) {
    ranges[0].label = "Days 1-3 (Menstrual)";
    ranges[0].arr = menstrual_1_3;
    ranges[0].count = menstrual_1_3_count;
    ranges[0].genPtr = menstrual_general;
    ranges[0].genCount = menstrual_general_count;

    ranges[1].label = "Days 4-5 (Menstrual)";
    ranges[1].arr = menstrual_4_5;
    ranges[1].count = menstrual_4_5_count;
    ranges[1].genPtr = menstrual_general;
    ranges[1].genCount = menstrual_general_count;

    ranges[2].label = "Days 6-8 (Follicular)";
    ranges[2].arr = follicular_6_8;
    ranges[2].count = follicular_6_8_count;
    ranges[2].genPtr = follicular_general;
    ranges[2].genCount = follicular_general_count;

    ranges[3].label = "Days 9-11 (Follicular)";
    ranges[3].arr = follicular_9_11;
    ranges[3].count = follicular_9_11_count;
    ranges[3].genPtr = follicular_general;
    ranges[3].genCount = follicular_general_count;

    ranges[4].label = "Days 12-13 (Follicular)";
    ranges[4].arr = follicular_12_13;
    ranges[4].count = follicular_12_13_count;
    ranges[4].genPtr = follicular_general;
    ranges[4].genCount = follicular_general_count;

    ranges[5].label = "Day 14 (Ovulation)";
    ranges[5].arr = ovulation_14;
    ranges[5].count = ovulation_14_count;
    ranges[5].genPtr = ovulation_general;
    ranges[5].genCount = ovulation_general_count;

    ranges[6].label = "Days 15-18 (Luteal)";
    ranges[6].arr = luteal_15_18;
    ranges[6].count = luteal_15_18_count;
    ranges[6].genPtr = luteal_general;
    ranges[6].genCount = luteal_general_count;

    ranges[7].label = "Days 19-22 (Luteal)";
    ranges[7].arr = luteal_19_22;
    ranges[7].count = luteal_19_22_count;
    ranges[7].genPtr = luteal_general;
    ranges[7].genCount = luteal_general_count;

    ranges[8].label = "Days 23-26 (Luteal)";
    ranges[8].arr = luteal_23_26;
    ranges[8].count = luteal_23_26_count;
    ranges[8].genPtr = luteal_general;
    ranges[8].genCount = luteal_general_count;

    ranges[9].label = "Days 27-28 (Luteal)";
    ranges[9].arr = luteal_27_28;
    ranges[9].count = luteal_27_28_count;
    ranges[9].genPtr = luteal_general;
    ranges[9].genCount = luteal_general_count;
}

int getRangeIndexForCycleDay(int cd, int cycleLen) {
    if (cd >=1 && cd <=3) return 0;
    if (cd >=4 && cd <=5) return 1;
    if (cd >=6 && cd <=8) return 2;
    if (cd >=9 && cd <=11) return 3;
    if (cd >=12 && cd <=13) return 4;
    if (cd == 14) return 5;
    if (cd >=15 && cd <=18) return 6;
    if (cd >=19 && cd <=22) return 7;
    if (cd >=23 && cd <=26) return 8;
    if (cd >=27 && cd <=28) return 9;
    if (cd >=29) return 9;
    return 9;
}

// ---------- Print helpers ----------
void printSymptomFreq(SymptomFreq *arr, int count, char printed[][60], int *printedCount) {
    for (int i=0;i<count;i++) {
        const char *sym = arr[i].symptom;
        if (!alreadyPrinted(sym, printed, *printedCount)) {
            printf("%s: %d-%d%%\n", sym, arr[i].low, arr[i].high);
            markPrinted(sym, printed, printedCount);
        }
    }
}

void printGeneral(const char **generalList, int genCount, char printed[][60], int *printedCount) {
    // print all general symptoms unconditionally, and mark them as printed
    for (int i=0;i<genCount;i++) {
        const char *sym = generalList[i];
        printf("%s\n", sym);
        markPrinted(sym, printed, printedCount);
    }
}

// ---------- Phase date prediction ----------
void printExpectedPhaseDates(int cycleDay, int cycleLength, struct tm lastPeriod) {
    printf("\n--- EXPECTED PHASE DATES ---\n");
    
    if (cycleDay <= 5) { // Menstrual Phase
        // Next phase: Follicular Phase (already started but shows when it ends and ovulation begins)
        struct tm FollicularDate = lastPeriod;
        addDaysToDate(&FollicularDate, 6); // Day 14
        printf("• Follicular Phase expected around: ");
        printFormattedDate(FollicularDate);
        printf("\n");
        
    } else if (cycleDay <= 12) { // Follicular Phase
        // Next phase: Ovulation
        struct tm ovulationDate = lastPeriod;
        addDaysToDate(&ovulationDate, 13); // Day 14
        printf("• Ovulation Phase expected around: ");
        printFormattedDate(ovulationDate);
        printf("\n");
        
    } else if (cycleDay <= 16) { // Ovulation Phase
        // Next phase: Luteal (already started)
        struct tm LutealDate = lastPeriod;
        addDaysToDate(&LutealDate, 16); //cycleLength
        printf("• Luteal Phase expected around: ");
        printFormattedDate(LutealDate);
        printf("\n");
        
    } else { // Luteal Phase
        // Next phase: Menstrual
        struct tm nextPeriod = lastPeriod;
        addDaysToDate(&nextPeriod, cycleLength);
        printf("• Next Menstrual Phase expected around: ");
        printFormattedDate(nextPeriod);
        printf("\n");
    }
}

// ---------- Helper function for result building ----------
static char resultBuffer[4096];

void appendResult(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    strcat(resultBuffer, buffer);
}

// ---------- Main logic: show current sub-range, phase general, next-3-days ----------
// Modified to accept symptom selection as parameter instead of scanf
char* runMenstrualTracker(int day, int month, int cycle, const char *personal_id, const char *symptomInput, int showMoreInfo) {
    resultBuffer[0] = '\0'; // Reset the buffer
    
    // --- GET TODAY'S DATE IN IST ---
    time_t t = time(NULL);
    t += 19800;  // IST offset
    struct tm *now = localtime(&t);
    int today = now->tm_mday;
    int thisMonth = now->tm_mon + 1;
    int thisYear = now->tm_year + 1900;

    // --- BUILD tm STRUCT FOR LAST PERIOD DATE ---
    struct tm lastPeriod = {0};
    lastPeriod.tm_mday = day;
    lastPeriod.tm_mon = month - 1;
    lastPeriod.tm_year = thisYear - 1900;
    struct tm todayDate = *now;

    // --- CALCULATE DAYS PASSED & cycle day ---
    int totalPassed = daysBetweenDates(lastPeriod, todayDate);
    int cycleDay = totalPassed + 1;

    if (totalPassed > cycle) {
        appendResult("\n⚠️ Your last period was over %d days ago.\n", totalPassed);
        appendResult("This may indicate an irregular cycle.\nPlease consult a doctor.\n");
        return resultBuffer;
    }

    char phase[50], advice[1200];
    int phaseIndex = -1;

    if (cycleDay <= 5) { 
    strcpy(phase, "Menstrual Phase"); 
    strcpy(advice,
        "\n1) Increase magnesium-rich foods (nuts, seeds, dark chocolate)\n"
        "   - Relaxes uterine muscles → reduces cramps, headaches, lower back pain.\n\n"

        "2) Eat Iron + Vitamin C together\n"
        "   - Bleeding lowers iron → iron rebuilds blood; Vitamin C increases absorption.\n\n"

        "3) Consume Omega-3 fatty acids (flaxseed, chia seeds, fish oil)\n"
        "   - Anti-inflammatory → eases cramps, fatigue, and headaches.\n\n"

        "4) Prefer warm, easy-to-digest meals (soups, khichdi, steamed veggies)\n"
        "   - Supports digestion → reduces bloating, diarrhea, and fatigue.\n\n"

        "5) Avoid caffeine & high-salt foods\n"
        "   - Caffeine increases cramps; salt worsens bloating.\n"
    );
    phaseIndex = 0;
    }

    else if (cycleDay <= 12) { 
    strcpy(phase, "Follicular Phase"); 
    strcpy(advice,
        "\n1) Increase high-quality protein (eggs, dal, paneer)\n"
        "   - Supports rising estrogen & hormone production.\n\n"

        "2) Add B-vitamins (whole grains, bananas, nuts)\n"
        "   - Boosts energy, mood, and focus.\n\n"

        "3) Eat antioxidant-rich foods (berries, fruits, vegetables)\n"
        "   - Helps skin clarity and reduces inflammation.\n\n"

        "4) Include healthy fats (avocado, nuts, olive oil)\n"
        "   - Stabilizes mood and supports hormonal balance.\n\n"

        "5) Stay well hydrated\n"
        "   - Helps formation of cervical mucus and improves energy.\n"
    );
    phaseIndex = 1;
    }

    else if (cycleDay <= 16) { 
    strcpy(phase, "Ovulation Phase"); 
    strcpy(advice,
        "\n1) Consume zinc-rich foods (pumpkin seeds, legumes)\n"
        "   - Supports ovulation and hormone regulation.\n\n"

        "2) Take Vitamin B6 + Magnesium\n"
        "   - Reduces ovulation pain (mittelschmerz) and breast tenderness.\n\n"

        "3) Increase water intake\n"
        "   - Maintains cervical mucus quality and reduces discomfort.\n\n"

        "4) Eat antioxidant-rich foods (citrus, berries, green tea)\n"
        "   - Helps reduce nausea and oxidative stress.\n\n"

        "5) Prefer light meals\n"
        "   - Prevents bloating and digestive discomfort during ovulation.\n"
    );
    phaseIndex = 2;
    }

    else { 
    strcpy(phase, "Luteal Phase"); 
    strcpy(advice,
        "\n1) Increase magnesium intake (spinach, nuts, seeds)\n"
        "   - Reduces PMS symptoms like cramps, irritability, and fatigue.\n\n"

        "2) Take Vitamin B6 foods (banana, potatoes, chicken)\n"
        "   - Helps stabilize mood swings and reduces anxiety.\n\n"

        "3) Eat complex carbs (oats, brown rice, fruits)\n"
        "   - Helps control cravings and supports stable energy.\n\n"

        "4) Add Omega-3 fats (flaxseed, walnuts)\n"
        "   - Reduces breast tenderness and headaches.\n\n"

        "5) Avoid sugary & processed foods\n"
        "   - Prevents bloating, mood swings, and fatigue.\n"
    );
    phaseIndex = 3;
    }

    // Build ranges
    RangeInfo ranges[RANGE_TOTAL];
    buildRanges(ranges);

    // Map current cycleDay to range index
    int curRange = getRangeIndexForCycleDay(cycleDay, cycle);

    // printed symptom buffer to avoid duplicates
    char printed[200][60]; int printedCount = 0;

    // --- OUTPUT current status ---
    appendResult("\n--- CURRENT STATUS ---\n");
    appendResult("Cycle Day: %d\n", cycleDay);
    appendResult("Cycle length: %d days\n", cycle);
    appendResult("Phase: %s\n", phase);
    appendResult("Advice: %s\n\n", advice);

    // Print current sub-range phrasing as requested
    appendResult("The common symptoms that you might experience on this day are:\n");
    // Print current sub-range symptoms (with percentages)
    for (int i=0;i<ranges[curRange].count;i++) {
        const char *sym = ranges[curRange].arr[i].symptom;
        if (!alreadyPrinted(sym, printed, printedCount)) {
            appendResult("%s: %d-%d%%\n", sym, ranges[curRange].arr[i].low, ranges[curRange].arr[i].high);
            markPrinted(sym, printed, &printedCount);
        }
    }

    // Print general phase symptoms (whole phase)
    appendResult("\nGeneral symptoms during this phase:\n");
    if (phaseIndex == 0) {
        for (int i=0;i<menstrual_general_count;i++) {
            const char *sym = menstrual_general[i];
            appendResult("%s\n", sym);
            markPrinted(sym, printed, &printedCount);
        }
    }
    else if (phaseIndex == 1) {
        for (int i=0;i<follicular_general_count;i++) {
            const char *sym = follicular_general[i];
            appendResult("%s\n", sym);
            markPrinted(sym, printed, &printedCount);
        }
    }
    else if (phaseIndex == 2) {
        for (int i=0;i<ovulation_general_count;i++) {
            const char *sym = ovulation_general[i];
            appendResult("%s\n", sym);
            markPrinted(sym, printed, &printedCount);
        }
    }
    else {
        for (int i=0;i<luteal_general_count;i++) {
            const char *sym = luteal_general[i];
            appendResult("%s\n", sym);
            markPrinted(sym, printed, &printedCount);
        }
    }

    // --- Next 3 days logic: collect unique symptoms from next 3 cycle days (no percentages, no repeats) ---
    appendResult("\nThe most expected symptoms for the next 3 days are:\n");

    // We'll collect unique symptom names (string without % part) and print them
    char nextPrinted[200][60]; int nextPrintedCount = 0;

    for (int k = 1; k <= 3; k++) {
        int nextDay = cycleDay + k;
        if (nextDay > cycle) nextDay = ((nextDay - 1) % cycle) + 1;
        int nextRange = getRangeIndexForCycleDay(nextDay, cycle);

        // for each symptom in that range
        for (int i = 0; i < ranges[nextRange].count; i++) {
            const char *raw = ranges[nextRange].arr[i].symptom;

            // Prepare stripped symptom (remove anything after '(' if present)
            char stripped[60];
            strncpy(stripped, raw, 59);
            stripped[59] = '\0';
            for (int c = 0; stripped[c] != '\0'; c++) {
                if (stripped[c] == '(') { stripped[c] = '\0'; break; }
            }
            // Trim trailing spaces
            int len = strlen(stripped);
            while (len > 0 && (stripped[len-1] == ' ' || stripped[len-1] == ',')) { stripped[len-1] = '\0'; len--; }

            if (len == 0) continue;

            // skip if already shown in current printed list (either current sub-range or general)
            if (alreadyPrinted(stripped, printed, printedCount)) continue;

            // skip if already added to nextPrinted
            int exists = 0;
            for (int p = 0; p < nextPrintedCount; p++) if (strcmp(stripped, nextPrinted[p]) == 0) { exists = 1; break; }
            if (exists) continue;

            // add to nextPrinted
            strncpy(nextPrinted[nextPrintedCount], stripped, 59);
            nextPrinted[nextPrintedCount][59] = '\0';
            nextPrintedCount++;
        }
    }

    if (nextPrintedCount == 0) {
        appendResult("No additional new symptoms expected (you might experience the same current symptoms).\n");
    } else {
        // print them as a single line, comma separated
        for (int i = 0; i < nextPrintedCount; i++) {
            appendResult("%s", nextPrinted[i]);
            if (i < nextPrintedCount - 1) appendResult(", ");
        }
        appendResult("\n");
    }
    
    // Process symptom input
    if (symptomInput && strlen(symptomInput) > 0) {
        appendResult("\nPlease select the symptoms you are experiencing today.\n");
        appendResult("Enter numbers separated by commas (e.g., 1,4,7). If everything is fine, select %d.\n", fixedSymCount);

        int userSymptoms[20], userSymptomCount = 0;
        char input[200];
        strcpy(input, symptomInput);
        char *token = strtok(input, ",");
        int everythingFineSelected = 0;
        while (token != NULL) {
            int num = atoi(token);
            if (num >= 1 && num <= fixedSymCount) {
                if (num == fixedSymCount) everythingFineSelected = 1; // "everything is fine"
                else userSymptoms[userSymptomCount++] = num - 1; // store index
            }
            token = strtok(NULL, ",");
        }

        // --- Display what the user selected ---
        if (everythingFineSelected) {
            appendResult("\nYou selected: Everything is fine.\n");
        } else if (userSymptomCount > 0) {
            appendResult("\nYou selected the following symptoms:\n");
            for (int i = 0; i < userSymptomCount; i++) {
                appendResult("- %s\n", fixedSymptoms[userSymptoms[i]]);
            }
        } else {
            appendResult("\nNo valid symptoms selected.\n");
        }
        
        // --- SAVE SYMPTOMS TO FILE ---
        updateUserSymptoms(personal_id, userSymptoms, userSymptomCount, everythingFineSelected);
        if(userSymptomCount != 0)
        appendResult("✅ Your symptoms have been saved for AI training!\n");
    }
    
    // --- Print expected phase dates ---
    appendResult("\n--- EXPECTED PHASE DATES ---\n");
    
    if (cycleDay <= 5) { // Menstrual Phase
        struct tm FollicularDate = lastPeriod;
        addDaysToDate(&FollicularDate, 6);
        appendResult("• Follicular Phase expected around: %d ", FollicularDate.tm_mday);
        char *months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
        appendResult("%s %d\n", months[FollicularDate.tm_mon], FollicularDate.tm_year + 1900);
        
    } else if (cycleDay <= 12) { // Follicular Phase
        struct tm ovulationDate = lastPeriod;
        addDaysToDate(&ovulationDate, 13);
        appendResult("• Ovulation Phase expected around: %d ", ovulationDate.tm_mday);
        char *months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
        appendResult("%s %d\n", months[ovulationDate.tm_mon], ovulationDate.tm_year + 1900);
        
    } else if (cycleDay <= 16) { // Ovulation Phase
        struct tm LutealDate = lastPeriod;
        addDaysToDate(&LutealDate, 16);
        appendResult("• Luteal Phase expected around: %d ", LutealDate.tm_mday);
        char *months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
        appendResult("%s %d\n", months[LutealDate.tm_mon], LutealDate.tm_year + 1900);
        
    } else { // Luteal Phase
        struct tm nextPeriod = lastPeriod;
        addDaysToDate(&nextPeriod, cycle);
        appendResult("• Next Menstrual Phase expected around: %d ", nextPeriod.tm_mday);
        char *months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
        appendResult("%s %d\n", months[nextPeriod.tm_mon], nextPeriod.tm_year + 1900);
    }
    
    // --- More info about phase ---
    if (showMoreInfo) {
        if (phaseIndex == 0) {
            const char *details[] = {
                "•The period is the first phase of the menstrual cycle. Estrogen and progesterone levels drop near the end of the previous cycle if pregnancy does not occur.",
                "•During menstruation, the uterus sheds the lining called the endometrium. Old blood and tissue exit the body through the vagina.",
                "•Periods vary widely. The average bleeding time for adults is 4.5 to 6 days. Typical blood loss ranges from 5 ml to 80 ml.",
                "•Tracking your cycle helps identify patterns and improves predictions for your next cycle.",
                "References:\n1. Varney's Midwifery (2019)\n2. The Lancet Child & Adolescent Health (2018)\n3. FIGO Recommendations (2011)"
            };
            int count = sizeof(details) / sizeof(details[0]);
            appendResult("\n--- More Information: Menstrual Phase ---\n");
            for (int i = 0; i < count; i++) appendResult("\n%s\n", details[i]);
        }
        else if (phaseIndex == 1) {
            const char *details[] = {
                "•The follicular phase begins right after menstruation and lasts until ovulation.",
                "•Follicle-stimulating hormone (FSH) rises, causing several follicles in the ovaries to mature. Usually, only one becomes dominant.",
                "•Estrogen levels steadily increase, improving mood, energy, focus, and skin quality.",
                "•The uterine lining (endometrium) starts to rebuild and thicken in preparation for possible pregnancy.",
                "•Exercise tolerance is higher during this phase, and strength or endurance workouts may feel easier.",
                "References:\n1. Guyton & Hall Textbook of Medical Physiology (2020)\n2. Endocrine Reviews Journal (2017)\n3. ACOG Clinical Guidelines (2019)"
            };
            int count = sizeof(details) / sizeof(details[0]);
            appendResult("\n--- More Information: Follicular Phase ---\n");
            for (int i = 0; i < count; i++) appendResult("\n%s\n", details[i]);
        }
        else if (phaseIndex == 2) {
            const char *details[] = {
                "•Ovulation occurs when a mature egg is released from the dominant ovarian follicle.",
                "•This usually happens about 10-16 days before the next period, depending on the cycle length.",
                "•Luteinizing hormone (LH) surges sharply, triggering the release of the egg within 24-36 hours.",
                "•Fertility is highest during this phase. Cervical mucus becomes clear, stretchy, and slippery (egg-white consistency).",
                "•Some individuals experience mild abdominal pain called 'mittelschmerz' on one side of the pelvis.",
                "References:\n1. Williams Gynecology (2020)\n2. Human Reproduction Update Journal (2016)\n3. ACOG Fertility Awareness Guidelines (2020)"
            };
            int count = sizeof(details) / sizeof(details[0]);
            appendResult("\n--- More Information: Ovulation Phase ---\n");
            for (int i = 0; i < count; i++) appendResult("\n%s\n", details[i]);
        }
        else {
            const char *details[] = {
                "•The luteal phase occurs after ovulation and lasts until the next period begins.",
                "•Progesterone levels rise significantly, which can lead to symptoms known as premenstrual syndrome (PMS).",
                "•Common PMS symptoms include headaches, breast tenderness, bloating, water retention, irritability, anxiety, tearfulness, fatigue, acne, and food cravings.",
                "•Some individuals may also experience positive changes such as increased creativity, higher libido, or improved confidence.",
                "•Tracking symptoms can help identify patterns and understand what factors worsen or ease them, such as sleep, diet, and stress levels.",
                "References:\n1. ACOG - Premenstrual Syndrome (2015)\n2. Gender Medicine Journal (2012)\n3. Feminism & Psychology Journal (2013)\n4. Women & Health Journal (2003)"
            };
            int count = sizeof(details) / sizeof(details[0]);
            appendResult("\n--- More Information: Luteal Phase ---\n");
            for (int i = 0; i < count; i++) appendResult("\n%s\n", details[i]);
        }
    }
    
    return resultBuffer;
}

// ---------- WebAssembly compatible main function (optional) ----------
// This can be called from JavaScript
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
char* wasm_register_user(const char* personal_id, const char* password, int day, int month, int cycle) {
    static char result[512];
    char outPersonalID[100], outPassword[100];
    int outDay, outMonth, outCycle;
    
    int status = registerUser(personal_id, password, day, month, cycle, outPersonalID, outPassword, &outDay, &outMonth, &outCycle);
    
    if (status == 1) {
        snprintf(result, sizeof(result), "SUCCESS: User %s registered successfully", personal_id);
    } else if (status == 0) {
        snprintf(result, sizeof(result), "ERROR: Invalid Personal ID");
    } else if (status == -1) {
        snprintf(result, sizeof(result), "ERROR: Personal ID already exists");
    } else if (status == -2) {
        snprintf(result, sizeof(result), "ERROR: Invalid month");
    } else if (status == -3) {
        snprintf(result, sizeof(result), "ERROR: Invalid day");
    } else if (status == -4) {
        snprintf(result, sizeof(result), "ERROR: Invalid cycle length (must be 21-35)");
    } else {
        snprintf(result, sizeof(result), "ERROR: Unknown error during registration");
    }
    
    return result;
}

EMSCRIPTEN_KEEPALIVE
char* wasm_login_user(const char* personal_id, const char* password) {
    static char result[512];
    char outPersonalID[100], outPassword[100];
    int outDay, outMonth, outCycle;
    
    int status = loginUser(personal_id, password, outPersonalID, outPassword, &outDay, &outMonth, &outCycle);
    
    if (status == 1) {
        snprintf(result, sizeof(result), "SUCCESS:%s:%d:%d:%d", personal_id, outDay, outMonth, outCycle);
    } else if (status == 0) {
        snprintf(result, sizeof(result), "ERROR: Personal ID not registered");
    } else if (status == -1) {
        snprintf(result, sizeof(result), "ERROR: Failed to load user data");
    } else if (status == -2) {
        snprintf(result, sizeof(result), "ERROR: Incorrect password");
    } else {
        snprintf(result, sizeof(result), "ERROR: Unknown error during login");
    }
    
    return result;
}

EMSCRIPTEN_KEEPALIVE
char* wasm_run_tracker(int day, int month, int cycle, const char* personal_id, const char* symptomInput, int showMoreInfo) {
    return runMenstrualTracker(day, month, cycle, personal_id, symptomInput, showMoreInfo);
}

EMSCRIPTEN_KEEPALIVE
int wasm_check_personal_id(const char* personal_id) {
    return isValidPersonalID(personal_id);
}

EMSCRIPTEN_KEEPALIVE
int wasm_personal_id_exists(const char* personal_id) {
    return personalIDExists(personal_id);
}
#endif

// Regular main function for testing
#ifndef __EMSCRIPTEN__
int main() {
    printf("🌺 Welcome to Menstrual Cycle Tracker 🌺\n\n");
    printf("This version is compiled for WebAssembly and requires JavaScript interface.\n");
    printf("Use the provided EMSCRIPTEN_KEEPALIVE functions from JavaScript.\n");
    return 0;
}
#endif