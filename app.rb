# app.rb
require 'sinatra/base'
require 'sinatra/json'
require 'aws-sdk'

module Tape
  class App < Sinatra::Base
    def bucket
      @bucket ||= Aws::S3::Resource.new(region: 'us-east-2').bucket('itmstore')
    end

    def error reason
      [500, {'Content-Type' => 'application/json'}, {error: reason}.to_json]
    end

    get '/' do
      erb :index
    end

    get '/uploads/new' do
      erb :upload
    end

    get '/uploads' do
      uploads = bucket.objects(delimiter: '/', prefix: 'todo/').map do |obj|
        parts = obj.key.split('/')
        parts.last if parts.count > 1
      end.compact

      json uploads: uploads
    end

    get '/uploads/:filename' do
      obj = bucket.object "todo/#{params[:filename]}"

      if obj.exists?
        file = Tempfile.new

        obj.get response_target: file.path

        send_file file.path,
          disposition: 'attachment',
          filename: params[:filename],
          type: 'application/octet-stream'
      else
        404
      end
    end

    post '/uploads' do
      filename = params[:file][:filename]
      file = params[:file][:tempfile]

      obj = bucket.object "todo/#{filename}"

      if obj.exists?
        @status = 400
        @error = "there is a file already queued with this name, \
                  is it possible this is a duplicate? If not (or you \
                  want a duplicate) please rename the file to something unique first."
      else
        if obj.upload_file file
          halt [200, 'ok']
        else
          @status = 500
          @error = 'there was a problem uploading the file'
        end
      end

      erb :error
    end

    post '/uploads/:filename/ok' do
      obj = bucket.object "todo/#{params[:filename]}"

      if obj.exists?
        obj.move_to "itmstore/done/#{params[:filename]}"

        [200, 'ok']
      else
        error 'obj ddn exs'
      end
    end

    get '/tapes' do
      tapes = bucket.objects(delimiter: '/', prefix: 'tapes/').map do |obj|
        parts = obj.key.split('/')
        parts.last if parts.count > 1
      end.compact

      json tapes: tapes
    end

    get '/tapes/:tape_id' do
      obj = bucket.object "tapes/#{params[:tape_id]}"

      if obj.exists?
        tape = JSON.parse obj.get.body.read

        json tape: tape
      else
        404
      end
    end

    post '/tapes' do
      tape = {
        name: params[:name],
        ticks: params[:ticks],
        side_a: {
          complete: false,
          tracks: []
        },
        side_b: {
          complete: false,
          tracks: []
        }
      }

      obj = bucket.object "tapes/#{params[:name]}"

      if obj.put body: JSON.pretty_generate(tape)
        json tape: tape
      else
        error 'o no'
      end
    end

    put '/tapes/:tape_id/side/:side' do
      obj = bucket.object "tapes/#{params[:tape_id]}"

      if obj.exists?
        tape = JSON.parse obj.get.body.read
        side = "side_#{params[:side]}"

        if params[:filename]
          next_position = (tape[side].collect { |a| a["position"] }.max || 0) + 1

          item = { position: next_position,
                   name: params[:filename],
                   ticks: params[:ticks].to_i }

          tape[side]['tracks'] << item
        end

        tape[side]['complete'] = params[:complete] if params[:complete]

        if obj.put body: JSON.pretty_generate(tape)
          json tape: tape
        else
          error 'o no'
        end
      else
        404
      end
    end
  end
end
