const SHEET_ID = '1DBuTAudAuJ234x3Qpmkm5QQPCmyd552QX35_sQ2UHIQ';
const DRIVE_ID = '1Acbc6KC3E3QkZdLhMZ6p69e_YWCrXBea';  

function getLastRowInColumn(sheet, column) {
  const values = sheet.getRange(1, column, sheet.getMaxRows()).getValues();
  for (let i = values.length - 1; i >= 0; i--) {
    if (values[i][0] !== "") {
      return i + 1;
    }
  }
  return 0;
}

function doPost(e) {
  var sheet = SpreadsheetApp.openById(SHEET_ID).getSheetByName("Timestamps");
  var timestamp = e.parameter.timestamp;
  var lastRow = getLastRowInColumn(sheet, 1); //last row in column A which is the timestamps

  if (timestamp && sheet) {
    var cell = sheet.getRange(lastRow + 1, 1);
    cell.setNumberFormat("@STRING@");
    cell.setValue(timestamp);  
    return ContentService.createTextOutput("Success");
  } else {
    return ContentService.createTextOutput("Missing timestamp or sheet");
  }
}

function doGet() {
  try {
    const tsSheet = SpreadsheetApp.openById(SHEET_ID).getSheetByName("Timestamps");
    const guiSheet = SpreadsheetApp.openById(SHEET_ID).getSheetByName("GUI");

    const lastRow = getLastRowInColumn(tsSheet, 1); // column A
    const startRow = Math.max(lastRow - 2, 1);       // get up to 3 most recent
    const ts = tsSheet.getRange(startRow, 1, lastRow - startRow + 1, 1).getValues().flat();

    const un = guiSheet.getRange("B3").getValue();                                                  // get unit number from guiSheet

    return ContentService.createTextOutput(
    JSON.stringify({
      timestamps: ts,   // send to ESP32
      unitNumber: un
    })
  ).setMimeType(ContentService.MimeType.JSON);

  } catch (err) {
    return ContentService.createTextOutput("Error: " + err.message);
  }
}

function matchByOrder() {
  const folder = DriveApp.getFolderById(DRIVE_ID);
  const files = folder.getFiles();
  const photos = [];

  // Collect and parse photo info
  while (files.hasNext()) {
    const file = files.next();
    const name = file.getName();
    const match = name.match(/^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.jpg$/);
    if (match) {
      const photoTimestamp = new Date(match[1].replace(' ', 'T'));
      photos.push({ date: photoTimestamp, url: file.getUrl() });
    }
  }

  // Sort ascending (older photos first)
  photos.sort((a, b) => a.date - b.date);

  const sheet = SpreadsheetApp.openById(SHEET_ID).getSheetByName("Timestamps");
  const rows = getLastRowInColumn(sheet, 1); // Timestamps in col A
  const timestampValues = sheet.getRange(1, 1, rows).getValues().flat();

  // Clear column B first
  sheet.getRange("B:B").clearContent();

  for (let i = 0; i < rows; i++) {
    const tsString = timestampValues[i];
    const ts = new Date(tsString.replace(' ', 'T'));
    let matchedIndex = -1;

    for (let j = 0; j < photos.length; j++) {
      const diff = (photos[j].date - ts) / 1000; // difference in seconds
      if (diff >= 1 && diff <= 30) { // 30 second window to check
        matchedIndex = j;
        break;
      }
    }

    const cell = sheet.getRange(i + 1, 2);
    if (matchedIndex !== -1) {
      const matchedPhoto = photos[matchedIndex];
      cell.setValue(matchedPhoto.url);
      photos.splice(matchedIndex, 1); // Remove the used photo to prevent reuse
    } else {
      cell.setValue("error");
    }
  }
}

