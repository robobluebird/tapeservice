require 'sinatra/base'
require 'sinatra/json'
require 'aws-sdk-s3'

module Tape
  class App < Sinatra::Base
    def bucket
      @bucket ||= Aws::S3::Resource.new(region: 'us-east-2').bucket('itmstore')
    end

    def error reason
      [500, {'Content-Type' => 'application/json'}, {error: reason}.to_json]
    end

    def obj
      @obj ||= begin
                 obj = bucket.object "tapes/#{params[:tape_id]}"
                 obj if obj.exists?
               end
    end

    def tape
      @tape ||= JSON.parse(obj.get.body.read) if obj
    end

    def queued
      if tape
        bucket.objects(delimiter: '/', prefix: "todo/#{tape['name']}/").count
      end
    end

    helpers do
      def side_a_status
        if tape['side_a']['complete']
          " (complete)"
        else
          if queued > 0
            " (<a href='/tapes/#{tape["name"]}/uploads'>#{queued} queued</a>)"
          else
            " (0 queued)"
          end
        end
      end

      def side_b_status
        if tape['side_a']['complete']
          if tape['side_b']['complete']
            " (complete)"
          else
            if queued > 0
              " (<a href='/tapes/#{tape["name"]}/uploads'>#{queued} queued</a>)"
            else
              " (0 queued)"
            end
          end
        else
          ""
        end
      end
    end

    before '/tapes/:tape_id*' do
      if tape.nil?
        if request.accept? 'text/html'
          @status = 404
          @error = 'tape not found!'
          halt erb :error
        else
          halt 404
        end
      end
    end

    get '/' do
      redirect to '/tapes'
    end

    get '/tapes/:tape_id/uploads/new' do
      tape
      erb :upload
    end

    get '/tapes/:tape_id/uploads' do
      @uploads = bucket.objects(delimiter: '/', prefix: "todo/#{params[:tape_id]}/").map do |obj|
        parts = obj.key.split('/')
        parts.last if parts.count > 1
      end.compact

      if request.accept? "text/html"
        tape
        erb :uploads
      else
        json uploads: uploads
      end
    end

    get '/tapes/:tape_id/uploads/:filename' do
      obj = bucket.object "todo/#{params[:tape_id]}/#{params[:filename]}"

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

    post '/tapes/:tape_id/uploads' do
      filename = params[:file][:filename]
      file = params[:file][:tempfile]

      obj = bucket.object "todo/#{tape['name']}/#{filename}"

      if obj.exists?
        @status = 400
        @error = "there is a file already queued with this name, \
                  is it possible this is a duplicate? If not (or you \
                  want a duplicate) please rename the file to something unique first."
      else
        if obj.upload_file file
          halt erb :uploaded
        else
          @status = 500
          @error = 'there was a problem uploading the file'
        end
      end

      erb :error
    end

    post '/tapes/:tape_id/uploads/:filename/ok' do
      obj = bucket.object "todo/#{params[:tape_id]}/#{params[:filename]}"

      if obj.exists?
        new_obj = obj.move_to "itmstore/done/#{params[:tape_id]}/#{params[:filename]}"

        if new_obj.exists? && !obj.exists?
          [200, 'ok']
        else
          error 'mv fld'
        end
      else
        error 'obj ddn exs'
      end
    end

    get '/tapes' do
      @tapes = bucket.objects(delimiter: '/', prefix: 'tapes/').map do |obj|
        parts = obj.key.split('/')
        parts.last if parts.count > 1
      end.compact

      if request.accept? 'text/html'
        erb :tapes
      else
        json tapes: @tapes
      end
    end

    post '/tapes' do
      halt json: { error: "taken" } if tape

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

    get "/commands" do
      if params[:password] && params[:password] == ENV["COMMAND_PASSWORD"]
        params[:tape_id] = "commands"
        json commands: tape
      end
    end

    post "/next_command" do
      if params[:password] && params[:password] == ENV["COMMAND_PASSWORD"]
        params[:tape_id] = "commands"

        command = tape["side_a"]["tracks"].shift || "none"

        tape["side_a"]["tracks"].each do |item|
          item["position"] = item["position"].to_i - 1
        end

        if obj.put body: JSON.pretty_generate(tape)
          json command: command
        else
          error 'o no'
        end
      else
        500
      end
    end

    post "/commands" do
      if params[:password] == ENV["command_password"]
        next_position = (tape[side]['tracks'].collect { |a| a["position"] }.max || 0) + 1

        item = { position: next_position,
                 name: params[:command],
                 ticks: 0 }

        tape[side]['tracks'] << item

        if obj.put body: JSON.pretty_generate(tape)
          json tape: tape
        else
          error 'o no'
        end
      end
    end

    get '/tapes/:tape_id/check' do
      200
    end

    get '/tapes/:tape_id' do
      if request.accept? 'text/html'
        tape
        erb :tape
      else
        json tape: tape
      end
    end

    put '/tapes/:tape_id/side/:side' do
      side = "side_#{params[:side]}"

      if params[:filename]
        next_position = (tape[side]['tracks'].collect { |a| a["position"] }.max || 0) + 1

        item = { position: next_position,
                 name: params[:filename],
                 ticks: params[:ticks].to_i }

        tape[side]['tracks'] << item
      end

      total_ticks = tape[side]['tracks'].reduce(0) { |mem, obj| mem += obj['ticks'].to_i }

      if params[:complete]
        tape[side]['complete'] = params[:complete]
      elsif total_ticks >= tape['ticks'].to_i
        tape[side]['complete'] = true
      end

      if obj.put body: JSON.pretty_generate(tape)
        json tape: tape
      else
        error 'o no'
      end
    end
  end
end
