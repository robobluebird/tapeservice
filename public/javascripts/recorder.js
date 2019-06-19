let record = document.querySelector('.record');
let stop = document.querySelector('.stop');
let submit = document.querySelector('.submit');
let soundClips = document.querySelector('.sound-clips');
let canvas = document.querySelector('.visualizer');
let mainSection = document.querySelector('.main-controls');

// disable stop button while not recording

stop.disabled = true;
submit.disabled = true;

//main block for doing the audio recording

if (navigator.mediaDevices.getUserMedia) {
  console.log('getUserMedia supported.');

  let constraints = { audio: true };
  let chunks = [];
  let blob = null;

  let onSuccess = (stream) => {
    var mediaRecorder = new MediaRecorder(stream);

    record.onclick = () => {
      mediaRecorder.start();
      console.log(mediaRecorder.state);
      console.log("recorder started");
      record.style.background = "red";

      stop.disabled = false;
      record.disabled = true;
      blob = null;
    }

    stop.onclick = () => {
      mediaRecorder.stop();
      console.log(mediaRecorder.state);
      console.log("recorder stopped");
      record.style.background = "";
      record.style.color = "";
      submit.style.background = "green";
      submit.disabled = false;

      stop.disabled = true;
      record.disabled = false;
    }

    submit.onclick = () => {
      if (blob) {
        var uploadFormData = new FormData();

        uploadFormData.append(
          "file",
          new Blob(
            [data],
            { type:"application/octet-stream" }
          )
        );

        $.ajax({
          url : "/tapes/<%= @tape['name'] %>/uploads",
          type : "POST",
          data : uploadFormData,
          cache : false,
          contentType : false,
          processData : false,
          success : (response) => {
            blob = null;
          }
        });
      }
    }

    mediaRecorder.onstop = (e) => {
      console.log("data available after MediaRecorder.stop() called.");
      blob = new Blob(chunks, { 'type' : 'audio/ogg; codecs=opus' });
      chunks = [];
    }

    mediaRecorder.ondataavailable = (e) => {
      chunks.push(e.data);
    }
  }

  let onError = (err) => {
    console.log('The following error occured: ' + err);
  }

  navigator.mediaDevices.getUserMedia(constraints).then(onSuccess, onError);
} else {
   console.log('getUserMedia not supported on your browser!');
}
