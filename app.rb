# app.rb
require 'sinatra/base'
require 'sinatra/json'
require 'aws-sdk'

module Tape
  class App < Sinatra::Base
    def bucket
      @bucket ||= Aws::S3::Resource.new(region: 'us-east-2').bucket('itmstore')
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

      if filename.end_with? '.wav', '.mp3'
        obj = bucket.object "todo/#{filename}"
        obj.upload_file file

        [200, 'ok']
      else
        [500, 'o no u ddn yus wav or mp3']
      end
    end

    post '/uploads/:filename/ok' do
      obj = bucket.object "todo/#{params[:filename]}"

      if obj.exists?
        obj.move_to "itmstore/done/#{params[:filename]}"

        [200, 'ok']
      else
        [500, 'obj ddn exs']
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
        file = Tempfile.new
        obj.get response_target: file.path
        tape = JSON.parse file.read

        json tape: tape
      else
        404
      end
    end

    post '/tapes' do
      tape = {
        name: params[:name],
        ticks: params[:ticks],
        side_a: [],
        side_b: []
      }

      file = Tempfile.new
      file.write JSON.pretty_generate tape

      obj = bucket.object "tapes/#{params[:name]}"
      obj.upload_file file

      json tape: tape
    end

    put '/tapes/:tape_id' do
      obj = bucket.object "tapes/#{params[:tape_id]}"

      if obj.exists?
        file = Tempfile.new
        obj.get response_target: file.path
        tape = JSON.parse file.read

        # add new info and re-up

        json tape: tape
      else
        404
      end
    end
  end
end
