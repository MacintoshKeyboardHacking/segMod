const char admin_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>segMod admin</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #submit {
        width:120px;
      }
      #edit_path {
        width:250px;
      }
      #delete_path {
        width:250px;
      }
      #spacer_50 {
        height: 50px;
      }
      #spacer_20 {
        height: 20px;
      }
      table {
        background-color: #dddddd;
        border-collapse: collapse;
        width:650px;
      }
      td, th {
        border: 1px solid #dddddd;
        text-align: left;
        padding: 8px;
      }
      #first_td_th {
        width:400px;
      }
      tr:nth-child(even) {
        background-color: #ffffff;
      }
      fieldset {
        width:700px;
        background-color: #f7f7f7;
      }
      #format_notice {
        color: #ff0000;
      }
    </style>
    <script>
      function validateFormUpdate()
      {
        var inputElement = document.getElementById('update');
        var files = inputElement.files;
        if(files.length==0)
        {
          alert("no file");
          return false;
        }
        var value = inputElement.value;
        var dotIndex = value.lastIndexOf(".")+1;
        var valueExtension = value.substring(dotIndex);
        if(valueExtension != "bin")
        {
          alert("wrong file");
          return false;
        }
      }
      function validateFormUpload()
      {
        var inputElement = document.getElementById('upload_data');
        var files = inputElement.files;
        if(files.length==0)
        {
          alert("no file");
          return false;
        }
      }
      function confirmFormat()
      {
        var text = "Pressing the \"OK\" button immediately deletes all data from SPIFFS and restarts ESP32!";
        if (confirm(text) == true) 
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    </script>
  </head>
  <body>
    <center>
      <h2>segMod admin</h2>

      <div id="spacer_20"></div>
      
      <fieldset>
        <legend>ESP32 update</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/update" enctype="multipart/form-data">
            <table><tr><td id="first_td_th">
            <input type="file" id="update" name="update">
            </td><td>
            <input type="submit" id="submit" value="doUpdate!" onclick="return validateFormUpdate()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>
      
      <fieldset>
        <legend>File upload</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/upload" enctype="multipart/form-data">
            <table><tr><td id="first_td_th">
            <input type="file" id="upload_data" name="upload_data">
            </td><td>
            <input type="submit" id="submit" value="doUpload!" onclick="return validateFormUpload()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>
      
      <div id="spacer_20"></div>

      <fieldset>
        <legend>SPIFFS format</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/format" target="self_page">
            <table><tr><td id="first_td_th">
            <p id="format_notice">Pressing the 'doFormat' button will immediately delete all data from SPIFFS!</p>
            </td><td>
            <input type="submit" id="submit" value="doFormat!" onclick="return confirmFormat()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_50"></div>

      <iframe style="display:none" name="self_page"></iframe>
    </center>
  </body>
</html> )rawliteral";

const char ok_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>update is success</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_20 {
        height: 20px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>update is success</h2>
      <div id="spacer_20"></div>
      <button onclick="window.location.href='/';">return</button>
    </center>
  </body>
</html> )rawliteral";

const char failed_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>update has fail</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_20 {
        height: 20px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>update has fail</h2>
      <div id="spacer_20"></div>
      <button onclick="window.location.href='/admin';">return</button>
    </center>
  </body>
</html> )rawliteral";
